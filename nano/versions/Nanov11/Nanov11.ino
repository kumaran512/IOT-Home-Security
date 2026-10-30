/*
  ============================================================
  IOT HOME SECURITY - NANO V8
  Live events | Non-blocking | Latest event takes priority
  ============================================================

  BOARD
    Arduino Nano ATmega328P

  OLED
    SSD1306 128x64 I2C
    Address: 0x3C
    SDA: A4
    SCL: A5

  SENSORS
    D2: Pedal sensor
    D3: IR motion sensor
    D4: Flame sensor

  LEDS
    D5: Motion LED
    D6: Pedal LED
    D7: Fire LED

  OUTPUTS
    D8: Vibration motor driver
    D9: Passive buzzer

  BEHAVIOUR
    - All sensors monitored continuously.
    - Most recently activated sensor controls the alert.
    - When that sensor clears, the newest remaining active
      sensor takes over immediately.
    - When no sensor is active, all alert outputs stop.
    - No delay() in the main application.
    - OLED animations and alert icons are preserved.
    - Motion/pedal: three 1400 Hz short beeps.
    - Fire: 1000-2200 Hz sweeping siren.
    - Motor: 500 ms pulse when a new event takes control.
    - LEDs blink independently for active sensors.

  LIBRARIES
    Adafruit SSD1306
    Adafruit GFX Library
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

// Power-failure simulation switch and status LED.
// Wire the toggle switch between D10 and GND.
// INPUT_PULLUP means LOW = power failure simulated.
const byte POWER_FAIL_SWITCH_PIN = 10;
const byte BATTERY_TAKEOVER_LED_PIN = 11;

// Most digital sensor modules are active LOW.
// Change to HIGH if your modules trigger HIGH.
const byte SENSOR_ACTIVE_STATE = LOW;

// ============================================================
// 3. TIMING CONFIGURATION
// ============================================================

const unsigned long DEBOUNCE_MS = 40UL;

const unsigned long MOTOR_ON_MS = 500UL;

const unsigned long IDLE_ANIMATION_MS = 180UL;
const unsigned long ALERT_ANIMATION_MS = 150UL;

const unsigned long LED_BLINK_MS = 250UL;
const unsigned long POWER_FAIL_DEBOUNCE_MS = 40UL;
const unsigned long POWER_FAIL_ANIMATION_MS = 180UL;

// Buzzer: three short beeps
const unsigned int BEEP_FREQUENCY = 1400;

const unsigned long BEEP_ON_MS  = 70UL;
const unsigned long BEEP_OFF_MS = 100UL;

// Fire siren
const unsigned int SIREN_MIN_FREQ = 1000;
const unsigned int SIREN_MAX_FREQ = 2200;
const unsigned int SIREN_STEP     = 100;

const unsigned long SIREN_STEP_MS = 25UL;

// ============================================================
// 4. EVENT TYPES
// ============================================================

enum EventType {
  EVENT_NONE,
  EVENT_PEDAL,
  EVENT_MOTION,
  EVENT_FIRE
};

EventType currentEvent = EVENT_NONE;

// ============================================================
// 5. SENSOR STATE
// ============================================================

struct SensorState {
  byte pin;

  bool rawActive;
  bool stableActive;

  unsigned long lastRawChange;
  unsigned long activatedAt;
};

SensorState pedalSensor = {
  PEDAL_SENSOR_PIN, false, false, 0, 0
};

SensorState motionSensor = {
  IR_SENSOR_PIN, false, false, 0, 0
};

SensorState flameSensor = {
  FLAME_SENSOR_PIN, false, false, 0, 0
};

// ============================================================
// 6. GLOBAL TIMERS
// ============================================================

unsigned long lastIdleFrame = 0;
unsigned long lastAlertFrame = 0;
unsigned long lastLedBlink = 0;

bool ledBlinkState = false;

// Power-failure simulation state (D10 toggle switch).
bool powerFailRawActive = false;
bool powerFailStableActive = false;
unsigned long powerFailLastRawChange = 0;
unsigned long lastPowerFailFrame = 0;
byte batteryAnimationFrame = 0;

// ============================================================
// 7. MOTOR STATE
// ============================================================

bool motorActive = false;
unsigned long motorStartTime = 0;

// ============================================================
// 8. BEEPER STATE
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
// 9. SIREN STATE
// ============================================================

bool sirenActive = false;

int sirenFrequency = SIREN_MIN_FREQ;
int sirenDirection = 1;

unsigned long lastSirenUpdate = 0;

// ============================================================
// 10. OLED ANIMATION STATE
// ============================================================

int eyeOffset = 0;
int eyeDirection = 1;

byte alertAnimationFrame = 0;

// ============================================================
// 11. FUNCTION DECLARATIONS
// ============================================================

void initializeSensors();

void updatePowerFailureSwitch();
void updateSensors();
void updateOneSensor(SensorState &sensor);

EventType selectLatestActiveEvent();
bool sensorIsActive(EventType eventType);

void changeEvent(EventType newEvent);
void startEventActions(EventType eventType);
void stopEventActions();

void startMotor();
void stopMotor();
void updateMotor();

void startBeepSequence();
void startSiren();
void updateBuzzer();
void updateSiren();
void stopSound();

void updateLEDs();

void updateDisplay();

void drawStartupScreen();
void drawPowerFailureScreen();
void drawBatteryIcon(byte frame);

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

  pinMode(POWER_FAIL_SWITCH_PIN, INPUT_PULLUP);
  pinMode(BATTERY_TAKEOVER_LED_PIN, OUTPUT);

  digitalWrite(IR_LED_PIN, LOW);
  digitalWrite(PEDAL_LED_PIN, LOW);
  digitalWrite(FIRE_LED_PIN, LOW);

  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(BATTERY_TAKEOVER_LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Serial.begin(9600);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    Serial.println(F("OLED initialization failed"));

    while (true) {
      // Halt if OLED initialization fails.
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.display();

  initializeSensors();

  // Initialize switch state without generating a false transition.
  powerFailRawActive =
    digitalRead(POWER_FAIL_SWITCH_PIN) == LOW;
  powerFailStableActive = powerFailRawActive;
  powerFailLastRawChange = millis();

  drawStartupScreen();

  // Initialize the idle animation without a blocking delay.
  lastIdleFrame = millis();
  lastAlertFrame = millis();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  // Read the power-failure simulation switch and sensors every loop.
  updatePowerFailureSwitch();
  updateSensors();

  // Select the most recently activated sensor that is
  // currently active. Switch events immediately if necessary.
  EventType selectedEvent = selectLatestActiveEvent();

  if (selectedEvent != currentEvent) {
    changeEvent(selectedEvent);
  }

  // Continue updating all non-blocking subsystems.
  updateMotor();
  updateBuzzer();
  updateLEDs();
  updateDisplay();
}

// ============================================================
// POWER-FAILURE SIMULATION SWITCH
// D10 LOW (switch closed to GND) = power failure / battery takeover.
// Debounced without delay().
// ============================================================

void updatePowerFailureSwitch() {
  unsigned long now = millis();
  bool reading = (digitalRead(POWER_FAIL_SWITCH_PIN) == LOW);

  if (reading != powerFailRawActive) {
    powerFailRawActive = reading;
    powerFailLastRawChange = now;
  }

  if (now - powerFailLastRawChange >= POWER_FAIL_DEBOUNCE_MS) {
    if (powerFailStableActive != powerFailRawActive) {
      powerFailStableActive = powerFailRawActive;
      lastPowerFailFrame = 0;
      batteryAnimationFrame = 0;

      if (powerFailStableActive) {
        Serial.println(F("POWER FAILURE SIMULATED - BATTERY TOOK OVER"));
        drawPowerFailureScreen();
      } else {
        Serial.println(F("MAIN POWER RESTORED"));
        // Restore the appropriate existing screen immediately.
        if (currentEvent == EVENT_NONE) {
          drawIdleScreen();
        } else {
          drawAlertScreen();
        }
      }
    }
  }

  digitalWrite(
    BATTERY_TAKEOVER_LED_PIN,
    powerFailStableActive ? HIGH : LOW
  );
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

  bool fireActive =
    digitalRead(FLAME_SENSOR_PIN) == SENSOR_ACTIVE_STATE;

  pedalSensor.rawActive = pedalActive;
  pedalSensor.stableActive = pedalActive;
  pedalSensor.lastRawChange = now;
  pedalSensor.activatedAt = pedalActive ? now : 0;

  motionSensor.rawActive = motionActive;
  motionSensor.stableActive = motionActive;
  motionSensor.lastRawChange = now;
  motionSensor.activatedAt = motionActive ? now : 0;

  flameSensor.rawActive = fireActive;
  flameSensor.stableActive = fireActive;
  flameSensor.lastRawChange = now;
  flameSensor.activatedAt = fireActive ? now : 0;
}

// ============================================================
// SENSOR MONITORING
// ============================================================

void updateSensors() {
  // Do not use else-if. All three sensors are checked
  // independently on every loop iteration.

  updateOneSensor(pedalSensor);
  updateOneSensor(motionSensor);
  updateOneSensor(flameSensor);
}

void updateOneSensor(SensorState &sensor) {
  unsigned long now = millis();

  bool reading =
    digitalRead(sensor.pin) == SENSOR_ACTIVE_STATE;

  if (reading != sensor.rawActive) {
    sensor.rawActive = reading;
    sensor.lastRawChange = now;
  }

  // Apply debounce to both activation and release.
  if (now - sensor.lastRawChange >= DEBOUNCE_MS) {

    if (sensor.stableActive != sensor.rawActive) {
      sensor.stableActive = sensor.rawActive;

      if (sensor.stableActive) {
        // Timestamp the activation only after debounce.
        sensor.activatedAt = now;
      }
      else {
        // Inactive sensors must not win event selection.
        sensor.activatedAt = 0;
      }
    }
  }
}

// ============================================================
// SELECT MOST RECENTLY ACTIVATED ACTIVE SENSOR
//
// If two sensors become active nearly simultaneously,
// the one whose debounced activation is processed last wins.
// Once selected, an event remains selected while its sensor
// stays active, unless a newer sensor activates.
//
// If the selected sensor clears, another active sensor
// immediately takes over.
// ============================================================

EventType selectLatestActiveEvent() {
  EventType selected = EVENT_NONE;
  unsigned long newestTime = 0;

  if (pedalSensor.stableActive) {
    selected = EVENT_PEDAL;
    newestTime = pedalSensor.activatedAt;
  }

  if (motionSensor.stableActive) {
    if (selected == EVENT_NONE ||
        motionSensor.activatedAt >= newestTime) {

      selected = EVENT_MOTION;
      newestTime = motionSensor.activatedAt;
    }
  }

  if (flameSensor.stableActive) {
    if (selected == EVENT_NONE ||
        flameSensor.activatedAt >= newestTime) {

      selected = EVENT_FIRE;
      newestTime = flameSensor.activatedAt;
    }
  }

  return selected;
}

// ============================================================
// CHANGE ACTIVE EVENT
//
// All actions associated with the old event stop immediately.
// Actions for the new event start immediately.
//
// If no sensors remain active, everything stops.
// ============================================================

void changeEvent(EventType newEvent) {
  // First stop the previous event's outputs.
  stopEventActions();

  currentEvent = newEvent;

  lastAlertFrame = 0;
  alertAnimationFrame = 0;

  if (currentEvent == EVENT_NONE) {
    Serial.println(F("ALL EVENTS CLEARED"));

    // Keep the power-failure page visible if it has priority.
    if (!powerFailStableActive) {
      drawIdleScreen();
    }

    return;
  }

  Serial.print(F("ACTIVE EVENT: "));

  switch (currentEvent) {
    case EVENT_PEDAL:
      Serial.println(F("PEDAL"));
      break;

    case EVENT_MOTION:
      Serial.println(F("MOTION"));
      break;

    case EVENT_FIRE:
      Serial.println(F("FIRE"));
      break;

    default:
      break;
  }

  startEventActions(currentEvent);

  // Show the new event immediately unless the power-failure
  // page currently has display priority.
  if (!powerFailStableActive) {
    drawAlertScreen();
  }
}

// ============================================================
// START EVENT ACTIONS
// ============================================================

void startEventActions(EventType eventType) {
  // Every new event starts with a fresh motor pulse.
  startMotor();

  if (eventType == EVENT_FIRE) {
    startSiren();
  }
  else if (eventType == EVENT_MOTION ||
           eventType == EVENT_PEDAL) {

    startBeepSequence();
  }
}

// ============================================================
// STOP EVENT ACTIONS
//
// Called on every event change and when all sensors clear.
// ============================================================

void stopEventActions() {
  // Stop any ongoing buzzer sequence or fire siren.
  stopSound();

  // Stop vibration immediately.
  stopMotor();
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
// NON-BLOCKING THREE-BEEP SEQUENCE
//
// 1400 Hz
// 70 ms ON, 100 ms OFF
// Three beeps total
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
// FIRE SIREN
//
// Sweeps between 1000 Hz and 2200 Hz while FIRE is the
// selected event. A change of event stops it immediately.
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
// INDEPENDENT LED BLINKING
//
// Each LED follows its own sensor's active state.
// The active event does not prevent another sensor's LED
// from blinking.
//
// When a sensor clears, its LED stops blinking after the
// debounce interval.
// ============================================================

void updateLEDs() {
  unsigned long now = millis();

  if (now - lastLedBlink >= LED_BLINK_MS) {
    lastLedBlink = now;
    ledBlinkState = !ledBlinkState;
  }

  digitalWrite(
    PEDAL_LED_PIN,
    pedalSensor.stableActive && ledBlinkState
      ? HIGH : LOW
  );

  digitalWrite(
    IR_LED_PIN,
    motionSensor.stableActive && ledBlinkState
      ? HIGH : LOW
  );

  digitalWrite(
    FIRE_LED_PIN,
    flameSensor.stableActive && ledBlinkState
      ? HIGH : LOW
  );
}

// ============================================================
// DISPLAY MANAGER
// ============================================================

void updateDisplay() {
  unsigned long now = millis();

  // Power-failure page has display priority while the D10 switch
  // is active. Sensor monitoring and alert outputs continue normally.
  if (powerFailStableActive) {
    if (now - lastPowerFailFrame >= POWER_FAIL_ANIMATION_MS) {
      lastPowerFailFrame = now;
      batteryAnimationFrame++;
      drawPowerFailureScreen();
    }
    return;
  }

  if (currentEvent == EVENT_NONE) {
    if (now - lastIdleFrame >= IDLE_ANIMATION_MS) {
      lastIdleFrame = now;
      updateIdleAnimation();
    }
    return;
  }

  if (now - lastAlertFrame >= ALERT_ANIMATION_MS) {
    lastAlertFrame = now;
    alertAnimationFrame++;
    drawAlertScreen();
  }
}

// ============================================================
// POWER FAILURE / BATTERY TAKEOVER OLED PAGE
// D10 switch active: this page overrides the display only.
// Existing sensor monitoring, LEDs, buzzer and vibration continue.
// ============================================================

void drawPowerFailureScreen() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(15, 1);
  display.print(F("POWER FAILURE"));

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  display.setCursor(8, 15);
  display.print(F("BATTERY TOOK OVER"));

  drawBatteryIcon(batteryAnimationFrame);

  display.setCursor(7, 54);
  display.print(F("BACKUP POWER ACTIVE"));

  display.display();
}

void drawBatteryIcon(byte frame) {
  // Battery body and terminal.
  const int x = 36;
  const int y = 30;
  const int w = 56;
  const int h = 17;

  display.drawRoundRect(x, y, w, h, 2, SSD1306_WHITE);
  display.fillRect(x + w, y + 5, 4, 7, SSD1306_WHITE);

  // Animated charging/takeover bars.
  // Three bars illuminate in sequence to indicate battery takeover.
  byte phase = frame % 4;
  for (byte i = 0; i < 4; i++) {
    int bx = x + 5 + (i * 12);
    if (i <= phase) {
      display.fillRect(bx, y + 4, 8, 9, SSD1306_WHITE);
    } else {
      display.drawRect(bx, y + 4, 8, 9, SSD1306_WHITE);
    }
  }

  // Animated arrow above the battery, moving left/right.
  int arrowX = (frame % 2 == 0) ? 58 : 70;
  display.drawLine(arrowX, 25, arrowX + 8, 25, SSD1306_WHITE);
  display.drawLine(arrowX + 5, 22, arrowX + 8, 25, SSD1306_WHITE);
  display.drawLine(arrowX + 5, 28, arrowX + 8, 25, SSD1306_WHITE);
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

  display.drawLine(
    12, 35, 115, 35, SSD1306_WHITE
  );

  display.setCursor(24, 43);
  display.print(F("SYSTEM READY"));

  display.setCursor(54, 53);
  display.print(F("V-D"));

  display.display();
}

// ============================================================
// IDLE SCREEN
//
// Fixed layout:
// Y 0-15  : Large DHARSSHITH title
// Y 19-47 : Compact animated eyes
// Y 53-63 : V-D footer
// ============================================================

void drawIdleScreen() {
  display.clearDisplay();

  drawIdleName();
  drawIdleEyes();
  drawIdleFooter();

  display.display();
}

void drawIdleName() {
  // Standard Adafruit font at size 2 makes the 10-character
  // name 120 pixels wide, using nearly the full 128-pixel display.
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 0);
  display.print(F("DHARSSHITH"));
}

void drawIdleFooter() {
  display.drawLine(
    0, 50, 127, 50, SSD1306_WHITE
  );

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(56, 55);
  display.print(F("V-D"));
}

void drawIdleEyes() {
  const int leftEyeX = 43;
  const int rightEyeX = 85;
  const int eyeY = 35;

  const int eyeWidth = 24;
  const int eyeHeight = 20;

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
// Fixed layout:
// Y 0-10  : Event title
// Y 13-40 : Icon
// Y 43-63 : Alert description
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

  display.drawLine(
    0, 11, 127, 11, SSD1306_WHITE
  );
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
// Uses supported Adafruit GFX drawing functions.
// ============================================================

void drawMotionIcon() {
  // Head
  display.drawCircle(
    64, 19, 5, SSD1306_WHITE
  );

  // Body
  display.drawLine(
    64, 24, 64, 32, SSD1306_WHITE
  );

  // Arms
  display.drawLine(
    64, 26, 54, 31, SSD1306_WHITE
  );

  display.drawLine(
    64, 26, 74, 31, SSD1306_WHITE
  );

  // Legs
  display.drawLine(
    64, 32, 57, 40, SSD1306_WHITE
  );

  display.drawLine(
    64, 32, 71, 40, SSD1306_WHITE
  );

  // Left movement indicators
  display.drawLine(
    43, 19, 49, 19, SSD1306_WHITE
  );

  display.drawLine(
    41, 25, 47, 27, SSD1306_WHITE
  );

  display.drawLine(
    43, 33, 49, 31, SSD1306_WHITE
  );

  // Right movement indicators
  display.drawLine(
    79, 19, 85, 19, SSD1306_WHITE
  );

  display.drawLine(
    81, 27, 87, 25, SSD1306_WHITE
  );

  display.drawLine(
    79, 31, 85, 33, SSD1306_WHITE
  );
}

// ============================================================
// FIRE ICON
// ============================================================

void drawFireIcon() {
  // Outer flame
  display.drawTriangle(
    64, 13,
    47, 36,
    81, 36,
    SSD1306_WHITE
  );

  // Inner flame
  display.drawTriangle(
    64, 24,
    56, 36,
    72, 36,
    SSD1306_WHITE
  );

  // Flame base
  display.drawLine(
    49, 39, 79, 39, SSD1306_WHITE
  );
}
