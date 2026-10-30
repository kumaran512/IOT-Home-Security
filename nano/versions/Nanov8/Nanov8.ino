/*
  ============================================================
  IOT HOME SECURITY - NANO V7
  Non-blocking multi-sensor event handling
  ============================================================

  HARDWARE
  -------
  Board       : Arduino Nano ATmega328P
  Display     : SSD1306 128x64 I2C OLED, address 0x3C

  SENSOR PINS
  -----------
  D2 : Pedal sensor
  D3 : IR motion sensor
  D4 : Flame sensor

  INDICATOR LEDS
  --------------
  D5 : IR / motion LED
  D6 : Pedal LED
  D7 : Fire LED

  OUTPUTS
  -------
  D8 : Vibration motor driver
  D9 : Passive buzzer

  FEATURES
  --------
  1. Non-blocking sensor monitoring
  2. Simultaneous sensor event capture
  3. Event priority: FIRE > MOTION > PEDAL
  4. Pending alerts displayed sequentially
  5. OLED idle animation and event icons
  6. Independent blinking event LEDs
  7. Three short beeps for pedal and motion
  8. Sweeping siren for fire
  9. Vibration motor for 500 ms per event
 10. Each event screen displayed for 8 seconds
 11. No delay() in the main application
 12. Compatible with Adafruit SSD1306 / GFX

  ============================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// 1. OLED CONFIGURATION
// ============================================================

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ============================================================
// 2. PIN CONFIGURATION
// ============================================================

const byte PEDAL_SENSOR_PIN = 2;
const byte IR_SENSOR_PIN    = 3;
const byte FLAME_SENSOR_PIN = 4;

const byte IR_LED_PIN       = 5;
const byte PEDAL_LED_PIN    = 6;
const byte FIRE_LED_PIN     = 7;

const byte MOTOR_PIN        = 8;
const byte BUZZER_PIN       = 9;

// Change to HIGH if your sensor modules trigger HIGH.
const byte SENSOR_ACTIVE_STATE = LOW;

// ============================================================
// 3. TIMING CONFIGURATION
// ============================================================

const unsigned long DEBOUNCE_MS       = 40UL;
const unsigned long EVENT_SCREEN_MS   = 8000UL;
const unsigned long MOTOR_ON_MS       = 500UL;

const unsigned long IDLE_ANIMATION_MS = 180UL;
const unsigned long ALERT_ANIMATION_MS = 150UL;
const unsigned long LED_BLINK_MS      = 250UL;

// Buzzer sequence
const unsigned int BEEP_FREQUENCY = 1400;
const unsigned long BEEP_ON_MS     = 70UL;
const unsigned long BEEP_OFF_MS    = 100UL;

// Fire siren
const unsigned int SIREN_MIN_FREQ = 1000;
const unsigned int SIREN_MAX_FREQ = 2200;
const unsigned int SIREN_STEP     = 100;

const unsigned long SIREN_STEP_MS = 25UL;

// ============================================================
// 4. EVENT TYPES AND PRIORITIES
// ============================================================

enum EventType {
  EVENT_NONE   = 0,
  EVENT_PEDAL  = 1,
  EVENT_MOTION = 2,
  EVENT_FIRE   = 3
};

// Pending-event bit flags.
const byte PENDING_PEDAL  = 0x01;
const byte PENDING_MOTION = 0x02;
const byte PENDING_FIRE   = 0x04;

byte pendingEvents = 0;

EventType currentEvent = EVENT_NONE;

// ============================================================
// 5. SENSOR STATE
// ============================================================

struct SensorState {
  byte pin;

  bool rawActive;
  bool stableActive;

  unsigned long lastRawChange;
};

SensorState pedalSensor = {
  PEDAL_SENSOR_PIN, false, false, 0
};

SensorState motionSensor = {
  IR_SENSOR_PIN, false, false, 0
};

SensorState flameSensor = {
  FLAME_SENSOR_PIN, false, false, 0
};

// ============================================================
// 6. GLOBAL TIMERS AND STATES
// ============================================================

unsigned long eventStartTime = 0;
unsigned long lastIdleFrame = 0;
unsigned long lastAlertFrame = 0;
unsigned long lastLedBlink = 0;

bool ledBlinkState = false;

bool motorActive = false;
unsigned long motorStartTime = 0;

// ============================================================
// 7. BEEPER STATE
// ============================================================

enum BeepPhase {
  BEEP_IDLE,
  BEEP_ON,
  BEEP_OFF
};

BeepPhase beepPhase = BEEP_IDLE;

byte completedBeeps = 0;
unsigned long beepPhaseStart = 0;

// ============================================================
// 8. SIREN STATE
// ============================================================

bool sirenActive = false;

int sirenFrequency = SIREN_MIN_FREQ;
int sirenDirection = 1;

unsigned long lastSirenUpdate = 0;

// ============================================================
// 9. IDLE ANIMATION STATE
// ============================================================

int eyeOffset = 0;
int eyeDirection = 1;

// ============================================================
// 10. FUNCTION DECLARATIONS
// ============================================================

void initializeSensors();
void updateSensors();
void updateOneSensor(SensorState &sensor, EventType eventType);

void queueEvent(EventType eventType);
void startNextEvent();
void finishCurrentEvent();

void updateMotor();
void startMotor();
void stopMotor();

void updateBuzzer();
void startBeepSequence();
void startSiren();
void stopSound();

void updateLEDs();
bool eventIsPendingOrCurrent(EventType eventType);

void updateDisplay();
void drawStartupScreen();

void drawIdleScreen();
void drawIdleName();
void drawIdleEyes();
void drawIdleFooter();
void updateIdleAnimation();

void drawAlertScreen();
void drawAlertHeader();
void drawAlertDescription();
void drawPedalIcon();
void drawMotionIcon();
void drawFireIcon();

byte eventToBit(EventType eventType);

// ============================================================
// SETUP
// ============================================================

void setup() {
  pinMode(PEDAL_SENSOR_PIN, INPUT_PULLUP);
  pinMode(IR_SENSOR_PIN, INPUT_PULLUP);
  pinMode(FLAME_SENSOR_PIN, INPUT_PULLUP);

  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(PEDAL_LED_PIN, OUTPUT);
  pinMode(FIRE_LED_PIN, OUTPUT);

  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(IR_LED_PIN, LOW);
  digitalWrite(PEDAL_LED_PIN, LOW);
  digitalWrite(FIRE_LED_PIN, LOW);

  digitalWrite(MOTOR_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.begin(9600);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    Serial.println(F("OLED initialization failed"));

    while (true) {
      // Halt if the display cannot initialize.
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.display();

  initializeSensors();

  drawStartupScreen();

  // No startup delay: the main loop remains responsive.
  drawIdleScreen();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  // Every subsystem is updated on every loop iteration.
  // No alert returns early and blocks sensor monitoring.

  updateSensors();

  // Start the highest-priority pending event if the
  // display is currently showing the idle screen.
  startNextEvent();

  updateMotor();
  updateBuzzer();
  updateLEDs();
  updateDisplay();
}

// ============================================================
// SENSOR INITIALIZATION
// ============================================================

void initializeSensors() {
  unsigned long now = millis();

  bool pedalActive =
    digitalRead(PEDAL_SENSOR_PIN) == SENSOR_ACTIVE_STATE;

  bool motionActive =
    digitalRead(IR_SENSOR_PIN) == SENSOR_ACTIVE_STATE;

  bool flameActive =
    digitalRead(FLAME_SENSOR_PIN) == SENSOR_ACTIVE_STATE;

  pedalSensor.rawActive = pedalActive;
  pedalSensor.stableActive = pedalActive;
  pedalSensor.lastRawChange = now;

  motionSensor.rawActive = motionActive;
  motionSensor.stableActive = motionActive;
  motionSensor.lastRawChange = now;

  flameSensor.rawActive = flameActive;
  flameSensor.stableActive = flameActive;
  flameSensor.lastRawChange = now;
}

// ============================================================
// SENSOR MONITORING
// ============================================================

void updateSensors() {
  // Read all sensors on every loop iteration.
  // Do not use else-if here: multiple sensors may trigger
  // during the same loop.

  updateOneSensor(pedalSensor, EVENT_PEDAL);
  updateOneSensor(motionSensor, EVENT_MOTION);
  updateOneSensor(flameSensor, EVENT_FIRE);
}

void updateOneSensor(
  SensorState &sensor,
  EventType eventType
) {
  unsigned long now = millis();

  bool reading =
    digitalRead(sensor.pin) == SENSOR_ACTIVE_STATE;

  // Detect a change in the raw sensor signal.
  if (reading != sensor.rawActive) {
    sensor.rawActive = reading;
    sensor.lastRawChange = now;
  }

  // Debounce the signal.
  if (now - sensor.lastRawChange >= DEBOUNCE_MS) {

    if (sensor.stableActive != sensor.rawActive) {
      bool previousState = sensor.stableActive;

      sensor.stableActive = sensor.rawActive;

      // Queue an event only on the inactive-to-active edge.
      // A sensor held active does not continuously enqueue
      // the same event.
      if (!previousState && sensor.stableActive) {
        queueEvent(eventType);
      }
    }
  }
}

// ============================================================
// EVENT QUEUE
// ============================================================

byte eventToBit(EventType eventType) {
  switch (eventType) {
    case EVENT_PEDAL:
      return PENDING_PEDAL;

    case EVENT_MOTION:
      return PENDING_MOTION;

    case EVENT_FIRE:
      return PENDING_FIRE;

    default:
      return 0;
  }
}

void queueEvent(EventType eventType) {
  byte eventBit = eventToBit(eventType);

  if (eventBit == 0) {
    return;
  }

  // Bit flags allow multiple different events to remain
  // pending at the same time.
  pendingEvents |= eventBit;

  switch (eventType) {
    case EVENT_FIRE:
      Serial.println(F("FIRE EVENT QUEUED"));
      break;

    case EVENT_MOTION:
      Serial.println(F("MOTION EVENT QUEUED"));
      break;

    case EVENT_PEDAL:
      Serial.println(F("PEDAL EVENT QUEUED"));
      break;

    default:
      break;
  }
}

// ============================================================
// START NEXT EVENT
//
// Priority order:
// 1. FIRE
// 2. MOTION
// 3. PEDAL
// ============================================================

void startNextEvent() {
  if (currentEvent != EVENT_NONE) {
    return;
  }

  if (pendingEvents == 0) {
    return;
  }

  if (pendingEvents & PENDING_FIRE) {
    currentEvent = EVENT_FIRE;
    pendingEvents &= ~PENDING_FIRE;
  }
  else if (pendingEvents & PENDING_MOTION) {
    currentEvent = EVENT_MOTION;
    pendingEvents &= ~PENDING_MOTION;
  }
  else if (pendingEvents & PENDING_PEDAL) {
    currentEvent = EVENT_PEDAL;
    pendingEvents &= ~PENDING_PEDAL;
  }

  if (currentEvent == EVENT_NONE) {
    return;
  }

  eventStartTime = millis();
  lastAlertFrame = 0;

  // Reset outputs belonging to the previous event.
  stopSound();

  // Start vibration without blocking the processor.
  startMotor();

  // Start the appropriate sound.
  if (currentEvent == EVENT_FIRE) {
    startSiren();
    Serial.println(F("DISPLAYING FIRE ALERT"));
  }
  else if (currentEvent == EVENT_MOTION) {
    startBeepSequence();
    Serial.println(F("DISPLAYING MOTION ALERT"));
  }
  else if (currentEvent == EVENT_PEDAL) {
    startBeepSequence();
    Serial.println(F("DISPLAYING PEDAL ALERT"));
  }

  // Draw the new event immediately.
  drawAlertScreen();
}

// ============================================================
// FINISH CURRENT EVENT
// ============================================================

void finishCurrentEvent() {
  stopSound();
  stopMotor();

  currentEvent = EVENT_NONE;

  // The next queued event will be selected on the next
  // pass through the main loop.
}

// ============================================================
// MOTOR CONTROL
// ============================================================

void startMotor() {
  digitalWrite(MOTOR_PIN, HIGH);

  motorActive = true;
  motorStartTime = millis();
}

void stopMotor() {
  digitalWrite(MOTOR_PIN, LOW);
  motorActive = false;
}

void updateMotor() {
  if (!motorActive) {
    return;
  }

  if (millis() - motorStartTime >= MOTOR_ON_MS) {
    stopMotor();
  }
}

// ============================================================
// NON-BLOCKING BEEPER
//
// Three short beeps:
// ON  70 ms
// OFF 100 ms
// ============================================================

void startBeepSequence() {
  completedBeeps = 0;

  beepPhase = BEEP_ON;
  beepPhaseStart = millis();

  tone(BUZZER_PIN, BEEP_FREQUENCY);
}

void updateBuzzer() {
  if (sirenActive) {
    updateSiren();
    return;
  }

  if (beepPhase == BEEP_IDLE) {
    return;
  }

  unsigned long now = millis();

  if (beepPhase == BEEP_ON) {
    if (now - beepPhaseStart >= BEEP_ON_MS) {
      noTone(BUZZER_PIN);

      completedBeeps++;
      beepPhase = BEEP_OFF;
      beepPhaseStart = now;

      // Three complete beeps.
      if (completedBeeps >= 3) {
        beepPhase = BEEP_IDLE;
      }
    }
  }
  else if (beepPhase == BEEP_OFF) {
    if (now - beepPhaseStart >= BEEP_OFF_MS) {
      tone(BUZZER_PIN, BEEP_FREQUENCY);

      beepPhase = BEEP_ON;
      beepPhaseStart = now;
    }
  }
}

// ============================================================
// NON-BLOCKING FIRE SIREN
// ============================================================

void startSiren() {
  sirenActive = true;

  sirenFrequency = SIREN_MIN_FREQ;
  sirenDirection = 1;

  lastSirenUpdate = millis();

  tone(BUZZER_PIN, sirenFrequency);
}

void updateSiren() {
  if (!sirenActive) {
    return;
  }

  unsigned long now = millis();

  if (now - lastSirenUpdate < SIREN_STEP_MS) {
    return;
  }

  lastSirenUpdate = now;

  sirenFrequency += sirenDirection * SIREN_STEP;

  if (sirenFrequency >= SIREN_MAX_FREQ) {
    sirenFrequency = SIREN_MAX_FREQ;
    sirenDirection = -1;
  }

  if (sirenFrequency <= SIREN_MIN_FREQ) {
    sirenFrequency = SIREN_MIN_FREQ;
    sirenDirection = 1;
  }

  tone(BUZZER_PIN, sirenFrequency);
}

void stopSound() {
  noTone(BUZZER_PIN);

  sirenActive = false;

  beepPhase = BEEP_IDLE;
  completedBeeps = 0;
}

// ============================================================
// LED MANAGEMENT
//
// LEDs blink independently.
// An LED blinks when its sensor is active, its event is
// being displayed, or its event is waiting in the queue.
// ============================================================

bool eventIsPendingOrCurrent(EventType eventType) {
  if (currentEvent == eventType) {
    return true;
  }

  return (pendingEvents & eventToBit(eventType)) != 0;
}

void updateLEDs() {
  unsigned long now = millis();

  if (now - lastLedBlink >= LED_BLINK_MS) {
    lastLedBlink = now;
    ledBlinkState = !ledBlinkState;
  }

  bool pedalLedRequired =
    pedalSensor.stableActive ||
    eventIsPendingOrCurrent(EVENT_PEDAL);

  bool motionLedRequired =
    motionSensor.stableActive ||
    eventIsPendingOrCurrent(EVENT_MOTION);

  bool fireLedRequired =
    flameSensor.stableActive ||
    eventIsPendingOrCurrent(EVENT_FIRE);

  digitalWrite(
    PEDAL_LED_PIN,
    pedalLedRequired && ledBlinkState ? HIGH : LOW
  );

  digitalWrite(
    IR_LED_PIN,
    motionLedRequired && ledBlinkState ? HIGH : LOW
  );

  digitalWrite(
    FIRE_LED_PIN,
    fireLedRequired && ledBlinkState ? HIGH : LOW
  );
}

// ============================================================
// DISPLAY MANAGER
// ============================================================

void updateDisplay() {
  unsigned long now = millis();

  if (currentEvent != EVENT_NONE) {

    // The event screen remains visible for its full duration.
    // Other sensors continue to be monitored in the meantime.
    if (now - eventStartTime >= EVENT_SCREEN_MS) {
      finishCurrentEvent();

      // Start the next pending event immediately if possible.
      startNextEvent();

      if (currentEvent == EVENT_NONE) {
        drawIdleScreen();
      }

      return;
    }

    // Animate the current alert without blocking sensor reads.
    if (now - lastAlertFrame >= ALERT_ANIMATION_MS) {
      lastAlertFrame = now;
      drawAlertScreen();
    }

    return;
  }

  // No event is currently being displayed.
  if (now - lastIdleFrame >= IDLE_ANIMATION_MS) {
    lastIdleFrame = now;
    updateIdleAnimation();
  }
}

// ============================================================
// STARTUP SCREEN
// ============================================================

void drawStartupScreen() {
  display.clearDisplay();

  display.drawRoundRect(
    1, 1, 126, 62, 5, SSD1306_WHITE
  );

  display.setTextSize(1);

  display.setCursor(23, 12);
  display.print(F("IOT HOME"));

  display.setCursor(29, 23);
  display.print(F("SECURITY"));

  display.drawLine(12, 35, 115, 35, SSD1306_WHITE);

  display.setCursor(24, 43);
  display.print(F("SYSTEM READY"));

  display.setCursor(54, 53);
  display.print(F("V-D"));

  display.display();
}

// ============================================================
// IDLE SCREEN
//
// Fixed screen regions:
// Y 0-10  : Name
// Y 13-47 : Animated eyes
// Y 50-63 : V-D footer
// ============================================================

void drawIdleScreen() {
  display.clearDisplay();

  drawIdleName();
  drawIdleEyes();
  drawIdleFooter();

  display.display();
}

void drawIdleName() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // The short name stays inside the top screen region.
  display.setCursor(31, 2);
  display.print(F("Dharsshith"));
}

void drawIdleFooter() {
  display.drawLine(0, 49, 127, 49, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(54, 53);
  display.print(F("V-D"));
}

void drawIdleEyes() {
  const int leftEyeX = 43;
  const int rightEyeX = 85;
  const int eyeY = 30;

  const int eyeWidth = 24;
  const int eyeHeight = 20;

  // Eye outlines.
  display.drawRoundRect(
    leftEyeX - eyeWidth / 2,
    eyeY - eyeHeight / 2,
    eyeWidth,
    eyeHeight,
    7,
    SSD1306_WHITE
  );

  display.drawRoundRect(
    rightEyeX - eyeWidth / 2,
    eyeY - eyeHeight / 2,
    eyeWidth,
    eyeHeight,
    7,
    SSD1306_WHITE
  );

  // Animated pupils.
  display.fillCircle(
    leftEyeX + eyeOffset,
    eyeY,
    4,
    SSD1306_WHITE
  );

  display.fillCircle(
    rightEyeX + eyeOffset,
    eyeY,
    4,
    SSD1306_WHITE
  );
}

void updateIdleAnimation() {
  eyeOffset += eyeDirection * 2;

  if (eyeOffset >= 5) {
    eyeOffset = 5;
    eyeDirection = -1;
  }
  else if (eyeOffset <= -5) {
    eyeOffset = -5;
    eyeDirection = 1;
  }

  drawIdleScreen();
}

// ============================================================
// ALERT SCREEN
//
// Fixed screen regions:
// Y 0-10  : Alert heading
// Y 13-40 : Icon
// Y 43-63 : Description
// ============================================================

void drawAlertScreen() {
  display.clearDisplay();

  drawAlertHeader();

  switch (currentEvent) {
    case EVENT_PEDAL:
      drawPedalIcon();
      break;

    case EVENT_MOTION:
      drawMotionIcon();
      break;

    case EVENT_FIRE:
      drawFireIcon();
      break;

    default:
      break;
  }

  drawAlertDescription();

  display.display();
}

void drawAlertHeader() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  switch (currentEvent) {
    case EVENT_PEDAL:
      display.setCursor(29, 2);
      display.print(F("PEDAL ALERT"));
      break;

    case EVENT_MOTION:
      display.setCursor(25, 2);
      display.print(F("MOTION ALERT"));
      break;

    case EVENT_FIRE:
      display.setCursor(35, 2);
      display.print(F("FIRE ALERT"));
      break;

    default:
      break;
  }

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
}

void drawAlertDescription() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  switch (currentEvent) {
    case EVENT_PEDAL:
      display.setCursor(19, 45);
      display.print(F("PEDAL ACTIVATED"));

      display.setCursor(34, 55);
      display.print(F("CHECK ENTRY"));
      break;

    case EVENT_MOTION:
      display.setCursor(25, 45);
      display.print(F("MOTION FOUND"));

      display.setCursor(31, 55);
      display.print(F("CHECK AREA"));
      break;

    case EVENT_FIRE:
      display.setCursor(35, 45);
      display.print(F("FIRE DETECTED"));

      display.setCursor(31, 55);
      display.print(F("CHECK AREA"));
      break;

    default:
      break;
  }
}

// ============================================================
// PEDAL ICON
// ============================================================

void drawPedalIcon() {
  display.drawRoundRect(
    43, 29, 42, 10, 2, SSD1306_WHITE
  );

  display.drawLine(
    48, 27, 80, 27, SSD1306_WHITE
  );

  display.drawLine(
    48, 27, 44, 21, SSD1306_WHITE
  );

  display.drawLine(
    80, 27, 84, 21, SSD1306_WHITE
  );

  display.drawLine(
    64, 27, 64, 18, SSD1306_WHITE
  );

  display.fillCircle(
    64, 17, 3, SSD1306_WHITE
  );

  display.drawLine(
    32, 21, 38, 21, SSD1306_WHITE
  );

  display.drawLine(
    90, 21, 96, 21, SSD1306_WHITE
  );
}

// ============================================================
// MOTION ICON
// Adafruit GFX supported functions only.
// ============================================================

void drawMotionIcon() {
  // Head
  display.drawCircle(64, 19, 5, SSD1306_WHITE);

  // Body
  display.drawLine(64, 24, 64, 32, SSD1306_WHITE);

  // Arms
  display.drawLine(64, 26, 54, 31, SSD1306_WHITE);
  display.drawLine(64, 26, 74, 31, SSD1306_WHITE);

  // Legs
  display.drawLine(64, 32, 57, 40, SSD1306_WHITE);
  display.drawLine(64, 32, 71, 40, SSD1306_WHITE);

  // Left movement indicators
  display.drawLine(43, 19, 49, 19, SSD1306_WHITE);
  display.drawLine(41, 25, 47, 27, SSD1306_WHITE);
  display.drawLine(43, 33, 49, 31, SSD1306_WHITE);

  // Right movement indicators
  display.drawLine(79, 19, 85, 19, SSD1306_WHITE);
  display.drawLine(81, 27, 87, 25, SSD1306_WHITE);
  display.drawLine(79, 31, 85, 33, SSD1306_WHITE);
}

// ============================================================
// FIRE ICON
// ============================================================

void drawFireIcon() {
  display.drawTriangle(
    64, 13,
    47, 36,
    81, 36,
    SSD1306_WHITE
  );

  display.drawTriangle(
    64, 24,
    56, 36,
    72, 36,
    SSD1306_WHITE
  );

  display.drawLine(
    49, 39, 79, 39, SSD1306_WHITE
  );
}
