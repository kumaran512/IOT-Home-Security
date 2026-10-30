/*
  IoT Based Home Security System
  Arduino Nano - Final Integration Test

  BOOT SCREEN:
    Dharsshith Kumaran
    V-D

  OLED:
    SDA -> A4
    SCL -> A5
    I2C address -> 0x3C

  INPUTS:
    Door sensor  -> D2
    IR sensor    -> D3
    Flame sensor -> D4

  INDICATOR LEDs:
    Door LED     -> D6
    IR LED       -> D5
    Fire LED     -> D7

  OUTPUTS:
    Vibration motor driver -> D8
    Piezo buzzer            -> D9

  EVENT BEHAVIOUR:
    Door event  -> Door LED + vibration
    IR event    -> IR LED + vibration
    Fire event  -> Fire LED + vibration + siren
    Motor       -> 500 ms per newly detected event

  IMPORTANT:
    Motor must use a suitable driver circuit.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED CONFIGURATION
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

bool oledAvailable = false;

// =====================================================
// PIN CONFIGURATION
// =====================================================

const byte doorPin = 2;
const byte irPin = 3;
const byte flamePin = 4;

const byte doorLedPin = 6;
const byte irLedPin = 5;
const byte fireLedPin = 7;

const byte motorPin = 8;
const byte buzzerPin = 9;

// =====================================================
// SENSOR POLARITY
// Change these if your modules are active-LOW.
// =====================================================

const byte DOOR_ACTIVE_STATE = LOW;
const byte IR_ACTIVE_STATE = LOW;
const byte FLAME_ACTIVE_STATE = LOW;

const bool MOTOR_ACTIVE_HIGH = true;

// =====================================================
// SENSOR STATES
// =====================================================

bool doorOpen = false;
bool irDetected = false;
bool fireDetected = false;

bool previousDoorOpen = false;
bool previousIrDetected = false;
bool previousFireDetected = false;

bool sensorsInitialized = false;

// =====================================================
// MOTOR EVENT CONTROL
// =====================================================

bool motorRunning = false;

unsigned long motorStartMillis = 0;

const unsigned long MOTOR_ON_DURATION = 500;

// =====================================================
// BUZZER SIREN
// =====================================================

unsigned long previousSirenMillis = 0;

const unsigned long SWEEP_INTERVAL = 2;

int currentFrequency = 1000;
int sweepDirection = 15;

bool sirenRunning = false;

// =====================================================
// OLED UPDATE TIMING
// =====================================================

unsigned long previousDisplayMillis = 0;

const unsigned long DISPLAY_INTERVAL = 150;

// =====================================================
// FIRE ANIMATION
// =====================================================

byte fireFrame = 0;

unsigned long previousFireAnimationMillis = 0;

const unsigned long FIRE_ANIMATION_INTERVAL = 250;

// =====================================================
// EVENT TRACKING
// =====================================================

bool eventActive = false;

// =====================================================
// MOTOR CONTROL
// =====================================================

void setMotor(bool on) {
  motorRunning = on;

  if (MOTOR_ACTIVE_HIGH) {
    digitalWrite(motorPin, on ? HIGH : LOW);
  } else {
    digitalWrite(motorPin, on ? LOW : HIGH);
  }
}

void triggerVibration() {

  // Extend the active vibration if another event occurs
  // while the motor is already running.
  motorStartMillis = millis();

  if (!motorRunning) {
    setMotor(true);
    Serial.println(F("EVENT: Vibration motor ON"));
  }
}

void updateMotor(unsigned long now) {

  if (motorRunning &&
      now - motorStartMillis >= MOTOR_ON_DURATION) {

    setMotor(false);

    Serial.println(F("EVENT: Vibration motor OFF"));
  }
}

// =====================================================
// READ SENSORS
// =====================================================

void readSensors() {

  doorOpen =
    (digitalRead(doorPin) == DOOR_ACTIVE_STATE);

  irDetected =
    (digitalRead(irPin) == IR_ACTIVE_STATE);

  fireDetected =
    (digitalRead(flamePin) == FLAME_ACTIVE_STATE);
}

// =====================================================
// LED CONTROL
// =====================================================

void updateLeds() {

  digitalWrite(
    doorLedPin,
    doorOpen ? HIGH : LOW
  );

  digitalWrite(
    irLedPin,
    irDetected ? HIGH : LOW
  );

  digitalWrite(
    fireLedPin,
    fireDetected ? HIGH : LOW
  );
}

// =====================================================
// EVENT DETECTION
// Vibrate only when a new event starts.
// =====================================================

void processEvents() {

  if (!sensorsInitialized) {

    previousDoorOpen = doorOpen;
    previousIrDetected = irDetected;
    previousFireDetected = fireDetected;

    sensorsInitialized = true;

    return;
  }

  bool newDoorEvent =
    doorOpen && !previousDoorOpen;

  bool newIrEvent =
    irDetected && !previousIrDetected;

  bool newFireEvent =
    fireDetected && !previousFireDetected;

  eventActive =
    doorOpen || irDetected || fireDetected;

  if (newDoorEvent) {
    Serial.println(F("ALERT: Door opened"));
  }

  if (newIrEvent) {
    Serial.println(F("ALERT: IR motion detected"));
  }

  if (newFireEvent) {
    Serial.println(F("ALERT: Flame detected"));
  }

  // Trigger vibration once for each newly detected event.
  if (newDoorEvent || newIrEvent || newFireEvent) {
    triggerVibration();
  }

  previousDoorOpen = doorOpen;
  previousIrDetected = irDetected;
  previousFireDetected = fireDetected;
}

// =====================================================
// FIRE ALARM SIREN
// =====================================================

void updateSiren(unsigned long now) {

  if (fireDetected) {

    if (!sirenRunning) {

      sirenRunning = true;

      currentFrequency = 1000;
      sweepDirection = 15;

      previousSirenMillis = now;

      tone(buzzerPin, currentFrequency);

      Serial.println(F("FIRE SIREN ON"));
    }

    if (now - previousSirenMillis >= SWEEP_INTERVAL) {

      previousSirenMillis = now;

      currentFrequency += sweepDirection;

      if (currentFrequency >= 2200) {
        currentFrequency = 2200;
        sweepDirection = -15;
      }

      if (currentFrequency <= 1000) {
        currentFrequency = 1000;
        sweepDirection = 15;
      }

      tone(buzzerPin, currentFrequency);
    }

  } else {

    if (sirenRunning) {

      noTone(buzzerPin);

      sirenRunning = false;

      Serial.println(F("FIRE SIREN OFF"));
    }
  }
}

// =====================================================
// DRAW FIRE ICON
// =====================================================

void drawFireIcon(int x, int y, byte frame) {

  display.fillTriangle(
    x + 8, y,
    x, y + 19,
    x + 16, y + 19,
    SSD1306_WHITE
  );

  display.fillCircle(
    x + 8, y + 15,
    8,
    SSD1306_WHITE
  );

  if (frame == 0) {

    display.fillTriangle(
      x + 8, y + 7,
      x + 4, y + 17,
      x + 12, y + 17,
      SSD1306_BLACK
    );

  } else {

    display.fillTriangle(
      x + 8, y + 5,
      x + 3, y + 17,
      x + 13, y + 17,
      SSD1306_BLACK
    );
  }
}

// =====================================================
// OLED FIRE ALERT SCREEN
// =====================================================

void drawFireScreen() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("!!! FIRE ALERT !!!"));

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  drawFireIcon(8, 25, fireFrame);

  display.setTextSize(2);
  display.setCursor(34, 22);
  display.println(F("FIRE"));

  display.setCursor(34, 40);
  display.println(F("DETECT"));

  if (fireFrame == 0) {
    display.fillCircle(118, 5, 4, SSD1306_WHITE);
  } else {
    display.drawCircle(118, 5, 4, SSD1306_WHITE);
  }

  display.display();
}

// =====================================================
// OLED NORMAL STATUS SCREEN
// =====================================================

void drawNormalScreen() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(F("HOME SECURITY"));

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  display.setCursor(0, 16);
  display.print(F("DOOR: "));
  display.println(doorOpen ? F("OPEN") : F("CLOSED"));

  display.setCursor(0, 28);
  display.print(F("IR: "));
  display.println(irDetected ? F("DETECTED") : F("CLEAR"));

  display.setCursor(0, 40);
  display.print(F("FIRE: "));
  display.println(fireDetected ? F("DETECTED") : F("CLEAR"));

  display.setCursor(0, 52);
  display.print(F("MOTOR: "));
  display.println(motorRunning ? F("ON") : F("OFF"));

  display.display();
}

// =====================================================
// OLED UPDATE
// =====================================================

void updateDisplay(unsigned long now) {

  if (!oledAvailable) {
    return;
  }

  if (now - previousDisplayMillis < DISPLAY_INTERVAL) {
    return;
  }

  previousDisplayMillis = now;

  if (fireDetected) {

    if (now - previousFireAnimationMillis >=
        FIRE_ANIMATION_INTERVAL) {

      previousFireAnimationMillis = now;

      fireFrame = !fireFrame;
    }

    drawFireScreen();

  } else {

    drawNormalScreen();
  }
}

// =====================================================
// BOOT SCREEN
// =====================================================

void showBootScreen() {

  if (!oledAvailable) {
    return;
  }

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(8, 15);
  display.println(F("Dharsshith Kumaran"));

  display.drawLine(8, 29, 119, 29, SSD1306_WHITE);

  display.setTextSize(2);

  display.setCursor(45, 38);
  display.println(F("V-D"));

  display.display();

  // Keep boot screen visible for two seconds.
  delay(2000);
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(9600);

  pinMode(doorPin, INPUT_PULLUP);
  pinMode(irPin, INPUT);
  pinMode(flamePin, INPUT);

  pinMode(doorLedPin, OUTPUT);
  pinMode(irLedPin, OUTPUT);
  pinMode(fireLedPin, OUTPUT);

  pinMode(motorPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  setMotor(false);
  noTone(buzzerPin);

  Wire.begin();

  Serial.println();
  Serial.println(F("============================"));
  Serial.println(F("HOME SECURITY SYSTEM"));
  Serial.println(F("============================"));

  if (display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    oledAvailable = true;

    Serial.println(F("OLED initialised successfully."));

    showBootScreen();

  } else {

    oledAvailable = false;

    Serial.println(F("OLED initialisation FAILED."));
  }

  readSensors();

  updateLeds();

  // Set initial states without triggering startup vibration.
  previousDoorOpen = doorOpen;
  previousIrDetected = irDetected;
  previousFireDetected = fireDetected;

  sensorsInitialized = true;

  unsigned long now = millis();

  previousDisplayMillis = now;
  previousFireAnimationMillis = now;

  Serial.println(F("System ready."));
  Serial.println(F("Motor: event-triggered only."));
  Serial.println(F("Buzzer: D9."));
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {

  unsigned long now = millis();

  readSensors();

  updateLeds();

  processEvents();

  updateMotor(now);

  updateSiren(now);

  updateDisplay(now);
}
