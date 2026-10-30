/*
  ============================================================
  IOT HOME SECURITY - NANO V6
  OLED layout corrected to prevent text/animation overlap
  Hardware: Arduino Nano ATmega328P
  Display: 128x64 SSD1306 I2C OLED, address 0x3C
  Libraries:
    - Adafruit SSD1306
    - Adafruit GFX Library
  ============================================================
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- OLED CONFIGURATION ----------------

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

// ---------------- PIN CONFIGURATION ----------------

const byte PEDAL_SENSOR_PIN = 2;
const byte IR_SENSOR_PIN    = 3;
const byte FLAME_SENSOR_PIN = 4;

const byte IR_LED_PIN       = 5;
const byte PEDAL_LED_PIN    = 6;
const byte FIRE_LED_PIN     = 7;

const byte MOTOR_PIN        = 8;
const byte BUZZER_PIN       = 9;

// Most digital sensor modules produce LOW when triggered.
// Change these to HIGH if your sensor modules work differently.
const byte SENSOR_ACTIVE_STATE = LOW;

// ---------------- TIMING ----------------

const unsigned long ALERT_SCREEN_DURATION = 8000UL;
const unsigned long DEBOUNCE_TIME          = 50UL;
const unsigned long IDLE_ANIMATION_INTERVAL = 180UL;
const unsigned long ALERT_ANIMATION_INTERVAL = 120UL;

// Buzzer pattern
const unsigned int BEEP_FREQUENCY = 1400;
const unsigned int BEEP_ON_TIME   = 70;
const unsigned int BEEP_OFF_TIME  = 100;

// Flame siren
const unsigned int SIREN_MIN_FREQ = 1000;
const unsigned int SIREN_MAX_FREQ = 2200;
const unsigned int SIREN_STEP     = 100;
const unsigned long SIREN_STEP_INTERVAL = 25UL;

// Vibration duration
const unsigned long MOTOR_DURATION = 500UL;

// ---------------- EVENT TYPES ----------------

enum EventType {
  EVENT_NONE,
  EVENT_PEDAL,
  EVENT_MOTION,
  EVENT_FIRE
};

EventType currentEvent = EVENT_NONE;

// ---------------- EVENT STATE ----------------

unsigned long eventStartTime = 0;
unsigned long lastIdleAnimation = 0;
unsigned long lastAlertAnimation = 0;
unsigned long motorStartTime = 0;
unsigned long lastSirenStep = 0;

bool motorActive = false;
bool alertScreenActive = false;

// ---------------- BEEPER STATE ----------------

byte beepCount = 0;
bool beepIsOn = false;
bool beepSequenceActive = false;
unsigned long beepTimer = 0;

// ---------------- SIREN STATE ----------------

bool sirenActive = false;
int sirenFrequency = SIREN_MIN_FREQ;
int sirenDirection = 1;

// ---------------- SENSOR DEBOUNCE ----------------

struct SensorState {
  byte pin;
  bool stableState;
  bool lastReading;
  unsigned long lastChange;
};

SensorState pedalState = {
  PEDAL_SENSOR_PIN, false, false, 0
};

SensorState irState = {
  IR_SENSOR_PIN, false, false, 0
};

SensorState flameState = {
  FLAME_SENSOR_PIN, false, false, 0
};

// ---------------- IDLE ANIMATION ----------------

int eyeOffset = 0;
int eyeDirection = 1;
byte idleFrame = 0;

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

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("SSD1306 initialization failed"));

    while (true) {
      // Stop if the OLED cannot be initialized.
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.display();

  initializeSensorStates();

  drawStartupScreen();
  delay(1200);

  drawIdleScreen();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
  updateSensors();
  updateMotor();
  updateBuzzer();

  if (alertScreenActive) {
    updateAlertScreen();

    if (millis() - eventStartTime >= ALERT_SCREEN_DURATION) {
      alertScreenActive = false;
      currentEvent = EVENT_NONE;

      stopBuzzer();
      drawIdleScreen();
    }

    return;
  }

  updateIdleAnimation();

  // Flame takes priority over motion and pedal events.
  if (sensorTriggered(flameState)) {
    triggerEvent(EVENT_FIRE);
  }
  else if (sensorTriggered(irState)) {
    triggerEvent(EVENT_MOTION);
  }
  else if (sensorTriggered(pedalState)) {
    triggerEvent(EVENT_PEDAL);
  }
}

// ============================================================
// SENSOR INITIALIZATION
// ============================================================

void initializeSensorStates() {
  bool pedalReading = digitalRead(PEDAL_SENSOR_PIN);
  bool irReading = digitalRead(IR_SENSOR_PIN);
  bool flameReading = digitalRead(FLAME_SENSOR_PIN);

  pedalState.stableState =
    (pedalReading == SENSOR_ACTIVE_STATE);

  pedalState.lastReading = pedalState.stableState;

  irState.stableState =
    (irReading == SENSOR_ACTIVE_STATE);

  irState.lastReading = irState.stableState;

  flameState.stableState =
    (flameReading == SENSOR_ACTIVE_STATE);

  flameState.lastReading = flameState.stableState;

  unsigned long now = millis();

  pedalState.lastChange = now;
  irState.lastChange = now;
  flameState.lastChange = now;
}

// ============================================================
// SENSOR UPDATES
// ============================================================

void updateSensors() {
  updateOneSensor(pedalState);
  updateOneSensor(irState);
  updateOneSensor(flameState);
}

void updateOneSensor(SensorState &sensor) {
  bool reading =
    (digitalRead(sensor.pin) == SENSOR_ACTIVE_STATE);

  if (reading != sensor.lastReading) {
    sensor.lastReading = reading;
    sensor.lastChange = millis();
  }

  if (millis() - sensor.lastChange >= DEBOUNCE_TIME) {
    sensor.stableState = reading;
  }
}

bool sensorTriggered(const SensorState &sensor) {
  return sensor.stableState;
}

// ============================================================
// EVENT HANDLING
// ============================================================

void triggerEvent(EventType eventType) {
  // Prevent repeated triggering while an alert is displayed.
  if (alertScreenActive) {
    return;
  }

  currentEvent = eventType;
  alertScreenActive = true;

  eventStartTime = millis();
  lastAlertAnimation = 0;

  stopBuzzer();
  startMotor();

  digitalWrite(IR_LED_PIN, LOW);
  digitalWrite(PEDAL_LED_PIN, LOW);
  digitalWrite(FIRE_LED_PIN, LOW);

  switch (eventType) {
    case EVENT_PEDAL:
      digitalWrite(PEDAL_LED_PIN, HIGH);
      startBeepSequence();
      Serial.println(F("PEDAL ALERT"));
      break;

    case EVENT_MOTION:
      digitalWrite(IR_LED_PIN, HIGH);
      startBeepSequence();
      Serial.println(F("MOTION DETECTED"));
      break;

    case EVENT_FIRE:
      digitalWrite(FIRE_LED_PIN, HIGH);
      startSiren();
      Serial.println(F("FIRE ALERT"));
      break;

    default:
      break;
  }

  drawAlertScreen();
}

// ============================================================
// MOTOR CONTROL
// ============================================================

void startMotor() {
  digitalWrite(MOTOR_PIN, HIGH);

  motorActive = true;
  motorStartTime = millis();
}

void updateMotor() {
  if (motorActive &&
      millis() - motorStartTime >= MOTOR_DURATION) {
    digitalWrite(MOTOR_PIN, LOW);
    motorActive = false;
  }
}

// ============================================================
// NORMAL ALERT BEEP SEQUENCE
// Three short beeps
// ============================================================

void startBeepSequence() {
  beepCount = 0;
  beepIsOn = false;
  beepSequenceActive = true;
  beepTimer = millis();

  // Start the first beep immediately.
  tone(BUZZER_PIN, BEEP_FREQUENCY);
  beepIsOn = true;
  beepCount++;
}

void updateBuzzer() {
  if (sirenActive) {
    updateSiren();
    return;
  }

  if (!beepSequenceActive) {
    return;
  }

  unsigned long now = millis();

  if (beepIsOn) {
    if (now - beepTimer >= BEEP_ON_TIME) {
      noTone(BUZZER_PIN);
      beepIsOn = false;
      beepTimer = now;

      if (beepCount >= 3) {
        beepSequenceActive = false;
      }
    }
  }
  else {
    if (now - beepTimer >= BEEP_OFF_TIME) {
      tone(BUZZER_PIN, BEEP_FREQUENCY);
      beepIsOn = true;
      beepTimer = now;
      beepCount++;
    }
  }
}

void stopBuzzer() {
  noTone(BUZZER_PIN);

  beepSequenceActive = false;
  beepIsOn = false;
  beepCount = 0;

  sirenActive = false;
}

// ============================================================
// FIRE SIREN
// ============================================================

void startSiren() {
  sirenActive = true;
  sirenFrequency = SIREN_MIN_FREQ;
  sirenDirection = 1;
  lastSirenStep = millis();

  tone(BUZZER_PIN, sirenFrequency);
}

void updateSiren() {
  if (!sirenActive) {
    return;
  }

  unsigned long now = millis();

  if (now - lastSirenStep < SIREN_STEP_INTERVAL) {
    return;
  }

  lastSirenStep = now;

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

// ============================================================
// STARTUP SCREEN
// ============================================================

void drawStartupScreen() {
  display.clearDisplay();

  // Border
  display.drawRoundRect(1, 1, 126, 62, 5, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(23, 12);
  display.print(F("IOT HOME"));

  display.setCursor(29, 23);
  display.print(F("SECURITY"));

  display.drawLine(12, 35, 115, 35, SSD1306_WHITE);

  display.setCursor(24, 43);
  display.print(F("SYSTEM READY"));

  display.setCursor(49, 53);
  display.print(F("V-D"));

  display.display();
}

// ============================================================
// IDLE SCREEN
//
// FIXED OLED ZONES:
// Y = 0..10  : Name
// Y = 13..47 : Animated eyes
// Y = 50..63 : V-D label
//
// Text and animation never share the same area.
// ============================================================

void drawIdleScreen() {
  display.clearDisplay();

  drawIdleName();
  drawIdleEyes();
  drawIdleFooter();

  display.display();
}

void drawIdleName() {
  // Name zone: top 11 pixels.
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Center "Dharsshith" (11 characters).
  display.setCursor(31, 2);
  display.print(F("Dharsshith"));
}

void drawIdleFooter() {
  // Footer zone: bottom 14 pixels.
  display.drawLine(0, 49, 127, 49, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(54, 53);
  display.print(F("V-D"));
}

void drawIdleEyes() {
  // Eye area is restricted to Y=14..46.
  // This keeps the animation clear of both text zones.

  const int leftEyeX = 43;
  const int rightEyeX = 85;
  const int eyeY = 30;

  const int eyeWidth = 24;
  const int eyeHeight = 20;

  // Outer eye shapes.
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

  // Pupils move horizontally.
  int pupilOffset = eyeOffset;

  display.fillCircle(
    leftEyeX + pupilOffset,
    eyeY,
    4,
    SSD1306_WHITE
  );

  display.fillCircle(
    rightEyeX + pupilOffset,
    eyeY,
    4,
    SSD1306_WHITE
  );
}

void updateIdleAnimation() {
  unsigned long now = millis();

  if (now - lastIdleAnimation < IDLE_ANIMATION_INTERVAL) {
    return;
  }

  lastIdleAnimation = now;

  eyeOffset += eyeDirection * 2;

  if (eyeOffset >= 5) {
    eyeOffset = 5;
    eyeDirection = -1;
  }
  else if (eyeOffset <= -5) {
    eyeOffset = -5;
    eyeDirection = 1;
  }

  idleFrame++;

  // Redraw only the idle screen.
  drawIdleScreen();
}

// ============================================================
// ALERT SCREEN
//
// FIXED OLED ZONES:
// Y = 0..10  : Alert title
// Y = 12..40 : Icon
// Y = 43..63 : Alert description
//
// The icon and text have separate drawing regions.
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

  if (currentEvent == EVENT_FIRE) {
    display.setCursor(35, 2);
    display.print(F("FIRE ALERT"));
  }
  else if (currentEvent == EVENT_MOTION) {
    display.setCursor(25, 2);
    display.print(F("MOTION ALERT"));
  }
  else if (currentEvent == EVENT_PEDAL) {
    display.setCursor(29, 2);
    display.print(F("PEDAL ALERT"));
  }

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);
}

void drawAlertDescription() {
  // Description zone begins below the icon area.
  // All text uses size 1 to fit the 128x64 screen.

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (currentEvent == EVENT_PEDAL) {
    display.setCursor(19, 45);
    display.print(F("PEDAL ACTIVATED"));

    display.setCursor(34, 55);
    display.print(F("CHECK ENTRY"));
  }
  else if (currentEvent == EVENT_MOTION) {
    display.setCursor(25, 45);
    display.print(F("MOTION FOUND"));

    display.setCursor(31, 55);
    display.print(F("CHECK AREA"));
  }
  else if (currentEvent == EVENT_FIRE) {
    display.setCursor(35, 45);
    display.print(F("FIRE DETECTED"));

    display.setCursor(31, 55);
    display.print(F("CHECK AREA"));
  }
}

// ============================================================
// PEDAL ICON
// Icon area: approximately X=0..127, Y=13..40
// ============================================================

void drawPedalIcon() {
  // Pedal base
  display.drawRoundRect(
    43, 29, 42, 10, 2, SSD1306_WHITE
  );

  // Pedal plate
  display.drawLine(48, 27, 80, 27, SSD1306_WHITE);
  display.drawLine(48, 27, 44, 21, SSD1306_WHITE);
  display.drawLine(80, 27, 84, 21, SSD1306_WHITE);

  // Pedal arm
  display.drawLine(64, 27, 64, 18, SSD1306_WHITE);

  // Pivot
  display.fillCircle(64, 17, 3, SSD1306_WHITE);

  // Movement marks
  display.drawLine(32, 21, 38, 21, SSD1306_WHITE);
  display.drawLine(90, 21, 96, 21, SSD1306_WHITE);
}

// ============================================================
// MOTION ICON
// Uses Adafruit GFX-supported drawing functions only.
// No drawArc() calls.
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

  // Left motion indicators
  display.drawLine(43, 19, 49, 19, SSD1306_WHITE);
  display.drawLine(41, 25, 47, 27, SSD1306_WHITE);
  display.drawLine(43, 33, 49, 31, SSD1306_WHITE);

  // Right motion indicators
  display.drawLine(79, 19, 85, 19, SSD1306_WHITE);
  display.drawLine(81, 27, 87, 25, SSD1306_WHITE);
  display.drawLine(79, 31, 85, 33, SSD1306_WHITE);
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

  // Base
  display.drawLine(49, 39, 79, 39, SSD1306_WHITE);
}

// ============================================================
// ALERT SCREEN ANIMATION
// ============================================================

void updateAlertScreen() {
  unsigned long now = millis();

  if (now - lastAlertAnimation < ALERT_ANIMATION_INTERVAL) {
    return;
  }

  lastAlertAnimation = now;

  // Flash the appropriate indicator LED.
  // The screen is redrawn from scratch each time, preventing
  // leftover pixels and animation/text overlap.

  if (currentEvent == EVENT_PEDAL) {
    digitalWrite(
      PEDAL_LED_PIN,
      !digitalRead(PEDAL_LED_PIN)
    );
  }
  else if (currentEvent == EVENT_MOTION) {
    digitalWrite(
      IR_LED_PIN,
      !digitalRead(IR_LED_PIN)
    );
  }
  else if (currentEvent == EVENT_FIRE) {
    digitalWrite(
      FIRE_LED_PIN,
      !digitalRead(FIRE_LED_PIN)
    );
  }

  drawAlertScreen();
}
