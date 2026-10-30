/*
  IoT Based Home Security System
  Arduino Nano - Final OLED Display Test

  OLED SSD1306 128x64 I2C:
    SDA -> A4
    SCL -> A5
    Address -> 0x3C

  SENSORS:
    Door sensor  -> D2 (active LOW)
    IR sensor    -> D3 (active LOW)
    Flame sensor -> D4 (active LOW)

  INDICATOR LEDs:
    IR LED       -> D5
    Door LED     -> D6
    Fire LED     -> D7

  OUTPUTS:
    Vibration motor driver -> D8
    Piezo buzzer           -> D9

  IDLE DISPLAY:
    Dharsshith Kumaran (large font)
    V-D

  EVENT DISPLAY:
    Door open, IR detected, or flame detected
    Shows the detected area for 5 seconds.

  MOTOR:
    Vibrates for 500 ms when a new event occurs.

  BUZZER:
    Sweeping siren while flame sensor remains active.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED
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

const byte irLedPin = 5;
const byte doorLedPin = 6;
const byte fireLedPin = 7;

const byte motorPin = 8;
const byte buzzerPin = 9;

// =====================================================
// SENSOR POLARITY
// =====================================================

const byte DOOR_ACTIVE_STATE = LOW;
const byte IR_ACTIVE_STATE = LOW;
const byte FLAME_ACTIVE_STATE = LOW;

const bool MOTOR_ACTIVE_HIGH = true;

// =====================================================
// TIMING
// =====================================================

const unsigned long MOTOR_ON_DURATION = 500;
const unsigned long EVENT_SCREEN_DURATION = 5000;

const unsigned long IDLE_ANIMATION_INTERVAL = 100;
const unsigned long EVENT_ANIMATION_INTERVAL = 250;

const unsigned long SWEEP_INTERVAL = 2;

// =====================================================
// SENSOR STATES
// =====================================================

bool doorOpen = false;
bool irDetected = false;
bool fireDetected = false;

bool previousDoorOpen = false;
bool previousIrDetected = false;
bool previousFireDetected = false;

// =====================================================
// MOTOR
// =====================================================

bool motorRunning = false;
unsigned long motorStartMillis = 0;

// =====================================================
// SIREN
// =====================================================

bool sirenRunning = false;

unsigned long previousSirenMillis = 0;

int currentFrequency = 1000;
int sweepDirection = 15;

// =====================================================
// DISPLAY STATE
// =====================================================

bool eventScreenActive = false;

unsigned long eventScreenStartMillis = 0;
unsigned long previousAnimationMillis = 0;

byte idleFrame = 0;
byte eventFrame = 0;

// Most recent event to display
enum EventType {
  EVENT_NONE,
  EVENT_DOOR,
  EVENT_IR,
  EVENT_FIRE
};

EventType displayedEvent = EVENT_NONE;

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

  motorStartMillis = millis();

  if (!motorRunning) {
    setMotor(true);
    Serial.println(F("Vibration motor ON"));
  }
}

void updateMotor(unsigned long now) {

  if (motorRunning &&
      now - motorStartMillis >= MOTOR_ON_DURATION) {

    setMotor(false);

    Serial.println(F("Vibration motor OFF"));
  }
}

// =====================================================
// SENSOR READING
// =====================================================

void readSensors() {

  doorOpen =
    digitalRead(doorPin) == DOOR_ACTIVE_STATE;

  irDetected =
    digitalRead(irPin) == IR_ACTIVE_STATE;

  fireDetected =
    digitalRead(flamePin) == FLAME_ACTIVE_STATE;
}

// =====================================================
// LED CONTROL
// =====================================================

void updateLeds() {

  digitalWrite(doorLedPin, doorOpen ? HIGH : LOW);
  digitalWrite(irLedPin, irDetected ? HIGH : LOW);
  digitalWrite(fireLedPin, fireDetected ? HIGH : LOW);
}

// =====================================================
// DISPLAY EVENT
// =====================================================

void showEvent(EventType event, unsigned long now) {

  displayedEvent = event;

  eventScreenActive = true;
  eventScreenStartMillis = now;

  eventFrame = 0;
  previousAnimationMillis = now;

  Serial.print(F("OLED event: "));

  switch (event) {

    case EVENT_DOOR:
      Serial.println(F("DOOR OPEN"));
      break;

    case EVENT_IR:
      Serial.println(F("IR MOTION DETECTED"));
      break;

    case EVENT_FIRE:
      Serial.println(F("FLAME DETECTED"));
      break;

    default:
      break;
  }
}

// =====================================================
// PROCESS SENSOR EVENTS
// =====================================================

void processEvents(unsigned long now) {

  bool newDoorEvent =
    doorOpen && !previousDoorOpen;

  bool newIrEvent =
    irDetected && !previousIrDetected;

  bool newFireEvent =
    fireDetected && !previousFireDetected;

  // Prioritise fire, then door, then IR when multiple
  // events begin in the same loop iteration.
  if (newFireEvent) {

    showEvent(EVENT_FIRE, now);
    triggerVibration();

  } else if (newDoorEvent) {

    showEvent(EVENT_DOOR, now);
    triggerVibration();

  } else if (newIrEvent) {

    showEvent(EVENT_IR, now);
    triggerVibration();
  }

  previousDoorOpen = doorOpen;
  previousIrDetected = irDetected;
  previousFireDetected = fireDetected;
}

// =====================================================
// FIRE SIREN
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
// DRAW LARGE IDLE SCREEN
// =====================================================

void drawIdleScreen(byte frame) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // Animated star points
  int starX1 = (frame * 3) % SCREEN_WIDTH;
  int starX2 = SCREEN_WIDTH - ((frame * 2) % SCREEN_WIDTH);

  display.drawPixel(starX1, 3, SSD1306_WHITE);
  display.drawPixel(starX2, 60, SSD1306_WHITE);
  display.drawPixel((starX1 + 47) % SCREEN_WIDTH, 8, SSD1306_WHITE);
  display.drawPixel((starX2 + 31) % SCREEN_WIDTH, 55, SSD1306_WHITE);

  // The name is centred using the largest practical
  // standard Adafruit_GFX font size that fits.
  //
  // At text size 1, 19 characters need approximately
  // 114 pixels. At size 2, the name will not fit.
  //
  // The name is split into two large lines for clarity.

  display.setTextSize(2);

  display.setCursor(4, 15);
  display.println(F("Dharsshith"));

  display.setCursor(19, 33);
  display.println(F("Kumaran"));

  // Animated divider
  int lineWidth = 30 + ((frame * 5) % 70);

  display.drawLine(
    (SCREEN_WIDTH - lineWidth) / 2,
    51,
    (SCREEN_WIDTH + lineWidth) / 2,
    51,
    SSD1306_WHITE
  );

  // Compact V-D label at the bottom
  display.setTextSize(1);

  display.setCursor(54, 55);
  display.print(F("V-D"));

  // Animated corner indicator
  if (frame % 4 < 2) {
    display.fillCircle(119, 6, 3, SSD1306_WHITE);
  } else {
    display.drawCircle(119, 6, 3, SSD1306_WHITE);
  }

  display.display();
}

// =====================================================
// DRAW ANIMATED FIRE SYMBOL
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
// DRAW EVENT SCREEN
// =====================================================

void drawEventScreen() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(F("HOME SECURITY ALERT"));

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  switch (displayedEvent) {

    case EVENT_DOOR:

      display.setTextSize(2);
      display.setCursor(5, 19);
      display.println(F("DOOR"));

      display.setCursor(5, 38);
      display.println(F("OPEN"));

      display.drawRect(94, 24, 24, 29, SSD1306_WHITE);

      if (eventFrame == 0) {
        display.drawLine(98, 28, 98, 49, SSD1306_WHITE);
      } else {
        display.drawLine(98, 28, 112, 49, SSD1306_WHITE);
      }

      break;

    case EVENT_IR:

      display.setTextSize(2);
      display.setCursor(5, 19);
      display.println(F("MOTION"));

      display.setCursor(5, 38);
      display.println(F("DETECTED"));

      display.drawCircle(105, 35, 5, SSD1306_WHITE);

      if (eventFrame == 0) {
        display.drawCircle(105, 35, 12, SSD1306_WHITE);
      } else {
        display.drawCircle(105, 35, 17, SSD1306_WHITE);
      }

      break;

    case EVENT_FIRE:

      drawFireIcon(8, 24, eventFrame);

      display.setTextSize(2);
      display.setCursor(34, 23);
      display.println(F("FIRE"));

      display.setCursor(34, 42);
      display.println(F("ALERT"));

      if (eventFrame == 0) {
        display.fillCircle(119, 5, 3, SSD1306_WHITE);
      } else {
        display.drawCircle(119, 5, 3, SSD1306_WHITE);
      }

      break;

    default:

      display.setTextSize(1);
      display.setCursor(10, 25);
      display.println(F("SECURITY EVENT"));

      break;
  }

  display.display();
}

// =====================================================
// OLED UPDATE
// =====================================================

void updateDisplay(unsigned long now) {

  if (!oledAvailable) {
    return;
  }

  if (eventScreenActive) {

    if (now - eventScreenStartMillis >=
        EVENT_SCREEN_DURATION) {

      eventScreenActive = false;

      idleFrame = 0;
      previousAnimationMillis = now;

      drawIdleScreen(idleFrame);

      return;
    }

    if (now - previousAnimationMillis >=
        EVENT_ANIMATION_INTERVAL) {

      previousAnimationMillis = now;

      eventFrame = !eventFrame;

      drawEventScreen();
    }

    return;
  }

  if (now - previousAnimationMillis >=
      IDLE_ANIMATION_INTERVAL) {

    previousAnimationMillis = now;

    idleFrame++;

    drawIdleScreen(idleFrame);
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(9600);

  pinMode(doorPin, INPUT_PULLUP);
  pinMode(irPin, INPUT);
  pinMode(flamePin, INPUT);

  pinMode(irLedPin, OUTPUT);
  pinMode(doorLedPin, OUTPUT);
  pinMode(fireLedPin, OUTPUT);

  pinMode(motorPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  setMotor(false);
  noTone(buzzerPin);

  Wire.begin();

  Serial.println();
  Serial.println(F("HOME SECURITY SYSTEM"));
  Serial.println(F("--------------------"));

  if (display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    oledAvailable = true;

    Serial.println(F("OLED initialised successfully."));

    drawIdleScreen(idleFrame);

  } else {

    oledAvailable = false;

    Serial.println(F("OLED initialisation FAILED."));
  }

  readSensors();
  updateLeds();

  // Prevent an already-active sensor at boot from
  // triggering an immediate vibration.
  previousDoorOpen = doorOpen;
  previousIrDetected = irDetected;
  previousFireDetected = fireDetected;

  unsigned long now = millis();

  previousAnimationMillis = now;

  Serial.println(F("System ready."));
  Serial.println(F("Idle: animated name screen."));
  Serial.println(F("Event screen duration: 5 seconds."));
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

  processEvents(now);

  updateMotor(now);

  updateSiren(now);

  updateDisplay(now);
}
