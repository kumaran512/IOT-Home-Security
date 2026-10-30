/*
  IoT Based Home Security System
  Arduino Nano - Final Animated OLED Firmware

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
    Piezo buzzer           -> D9

  DISPLAY:
    Animated idle screen:
      Dharsshith Kumaran
      V-D

    Event screen:
      Appears for 2 seconds after a new event.
      Then returns to the animated idle screen.

  EVENTS:
    Door opened -> LED + 500 ms vibration
    IR detected -> LED + 500 ms vibration
    Flame       -> LED + vibration + sweeping siren

  Motor must be connected through a suitable driver.
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
// Preserved from your working Nanov2.ino.
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
// MOTOR CONTROL
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
// DISPLAY TIMING
// =====================================================

const unsigned long EVENT_SCREEN_DURATION = 2000;
const unsigned long IDLE_ANIMATION_INTERVAL = 80;
const unsigned long EVENT_ANIMATION_INTERVAL = 200;

unsigned long eventScreenStartMillis = 0;
unsigned long previousAnimationMillis = 0;

bool eventScreenActive = false;

byte idleFrame = 0;
byte eventFrame = 0;

// =====================================================
// EVENT INFORMATION
// =====================================================

bool doorEventShown = false;
bool irEventShown = false;
bool fireEventShown = false;

// =====================================================
// MOTOR HELPERS
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

  // Restart the 500 ms duration if another event
  // occurs while the motor is already running.
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
    digitalRead(doorPin) == DOOR_ACTIVE_STATE;

  irDetected =
    digitalRead(irPin) == IR_ACTIVE_STATE;

  fireDetected =
    digitalRead(flamePin) == FLAME_ACTIVE_STATE;
}

// =====================================================
// UPDATE INDICATOR LEDS
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
// SHOW EVENT SCREEN
// =====================================================

void showEventScreen(unsigned long now) {

  eventScreenActive = true;
  eventScreenStartMillis = now;

  eventFrame = 0;
  previousAnimationMillis = now;

  // Clear the event labels from the previous event.
  doorEventShown = false;
  irEventShown = false;
  fireEventShown = false;
}

// =====================================================
// PROCESS NEW EVENTS
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

  if (newDoorEvent) {

    Serial.println(F("ALERT: Door opened"));

    doorEventShown = true;
  }

  if (newIrEvent) {

    Serial.println(F("ALERT: IR motion detected"));

    irEventShown = true;
  }

  if (newFireEvent) {

    Serial.println(F("ALERT: Flame detected"));

    fireEventShown = true;
  }

  // Any new event starts or restarts the 2-second
  // OLED event-screen timer.
  if (newDoorEvent || newIrEvent || newFireEvent) {

    showEventScreen(millis());

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
// DRAW ANIMATED FIRE ICON
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

  // Header
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (fireDetected) {
    display.println(F("!!! FIRE ALERT !!!"));
  } else {
    display.println(F("SECURITY EVENT"));
  }

  display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

  if (fireDetected) {

    drawFireIcon(7, 25, eventFrame);

    display.setTextSize(2);

    display.setCursor(34, 22);
    display.println(F("FIRE"));

    display.setCursor(34, 40);
    display.println(F("DETECT"));

    if (eventFrame == 0) {
      display.fillCircle(118, 5, 4, SSD1306_WHITE);
    } else {
      display.drawCircle(118, 5, 4, SSD1306_WHITE);
    }

  } else {

    display.setTextSize(1);

    display.setCursor(0, 16);

    if (doorEventShown) {
      display.println(F("DOOR OPENED"));
    }

    display.setCursor(0, 30);

    if (irEventShown) {
      display.println(F("IR DETECTED"));
    }

    display.setCursor(0, 44);

    if (motorRunning) {
      display.println(F("VIBRATION ALERT"));
    } else {
      display.println(F("CHECK SECURITY"));
    }

    // Animated alert symbol
    if (eventFrame == 0) {
      display.fillCircle(116, 38, 7, SSD1306_WHITE);
      display.fillCircle(116, 38, 3, SSD1306_BLACK);
    } else {
      display.drawCircle(116, 38, 7, SSD1306_WHITE);
    }
  }

  display.display();
}

// =====================================================
// DRAW ANIMATED IDLE / BOOT SCREEN
// =====================================================

void drawIdleScreen(byte frame) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // Animated moving stars
  int starX1 = (frame * 3) % SCREEN_WIDTH;
  int starX2 = (SCREEN_WIDTH - ((frame * 2) % SCREEN_WIDTH));

  display.drawPixel(starX1, 4, SSD1306_WHITE);
  display.drawPixel(starX2, 58, SSD1306_WHITE);
  display.drawPixel((starX1 + 43) % SCREEN_WIDTH, 10, SSD1306_WHITE);
  display.drawPixel((starX2 + 29) % SCREEN_WIDTH, 53, SSD1306_WHITE);

  // Animated border
  if (frame % 2 == 0) {

    display.drawRoundRect(
      3, 12, 122, 39, 6, SSD1306_WHITE
    );

  } else {

    display.drawRoundRect(
      1, 10, 126, 43, 7, SSD1306_WHITE
    );
  }

  // Name
  display.setTextSize(1);

  display.setCursor(17, 22);
  display.println(F("Dharsshith Kumaran"));

  // Animated divider
  int lineWidth = 20 + ((frame * 7) % 90);

  display.drawLine(
    (SCREEN_WIDTH - lineWidth) / 2,
    34,
    (SCREEN_WIDTH + lineWidth) / 2,
    34,
    SSD1306_WHITE
  );

  // V-D label
  display.setTextSize(2);

  display.setCursor(45, 38);
  display.println(F("V-D"));

  // Small animated indicator
  if (frame % 4 < 2) {
    display.fillCircle(115, 5, 3, SSD1306_WHITE);
  } else {
    display.drawCircle(115, 5, 3, SSD1306_WHITE);
  }

  display.display();
}

// =====================================================
// OLED DISPLAY MANAGER
// =====================================================

void updateDisplay(unsigned long now) {

  if (!oledAvailable) {
    return;
  }

  // An event screen remains visible for two seconds
  // after the most recent new event.
  if (eventScreenActive) {

    if (now - eventScreenStartMillis >=
        EVENT_SCREEN_DURATION) {

      eventScreenActive = false;

      // Restart the idle animation smoothly.
      idleFrame = 0;
      previousAnimationMillis = now;

      drawIdleScreen(idleFrame);

      return;
    }

    // Animate the event screen.
    if (now - previousAnimationMillis >=
        EVENT_ANIMATION_INTERVAL) {

      previousAnimationMillis = now;

      eventFrame = !eventFrame;

      drawEventScreen();
    }

    return;
  }

  // Normal idle animation.
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

  // Sensor inputs
  pinMode(doorPin, INPUT_PULLUP);
  pinMode(irPin, INPUT);
  pinMode(flamePin, INPUT);

  // Indicator LEDs
  pinMode(doorLedPin, OUTPUT);
  pinMode(irLedPin, OUTPUT);
  pinMode(fireLedPin, OUTPUT);

  // Actuator outputs
  pinMode(motorPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  setMotor(false);
  noTone(buzzerPin);

  // I2C
  Wire.begin();

  Serial.println();
  Serial.println(F("============================"));
  Serial.println(F("HOME SECURITY SYSTEM"));
  Serial.println(F("============================"));

  // Initialise OLED
  if (display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS)) {

    oledAvailable = true;

    Serial.println(F("OLED initialised successfully."));

    // Start with the animated idle screen.
    drawIdleScreen(idleFrame);

  } else {

    oledAvailable = false;

    Serial.println(F("OLED initialisation FAILED."));
  }

  // Read initial sensor states.
  readSensors();

  updateLeds();

  // Initial active sensors do not trigger startup vibration.
  previousDoorOpen = doorOpen;
  previousIrDetected = irDetected;
  previousFireDetected = fireDetected;

  sensorsInitialized = true;

  unsigned long now = millis();

  previousAnimationMillis = now;

  Serial.println(F("System ready."));
  Serial.println(F("OLED: animated idle screen."));
  Serial.println(F("Event screen: 2 seconds."));
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
