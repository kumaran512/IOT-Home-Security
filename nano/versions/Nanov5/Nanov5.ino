/*
  IoT Based Home Security System
  Arduino Nano - Pedal Sensor Integration

  OLED SSD1306 128x64:
    SDA -> A4
    SCL -> A5
    I2C address -> 0x3C

  INPUTS:
    Pedal sensor -> D2 (active LOW)
    IR sensor    -> D3 (active LOW)
    Flame sensor -> D4 (active LOW)

  LED INDICATORS:
    IR LED       -> D5
    Pedal LED    -> D6
    Fire LED     -> D7

  OUTPUTS:
    Vibration motor driver -> D8
    Piezo buzzer           -> D9

  EVENT BEHAVIOUR:
    Pedal pressed -> OLED alert + LED + vibration + quiet beep
    IR detected   -> OLED alert + LED + vibration + quiet beep
    Flame         -> OLED alert + LED + vibration + siren

  OLED:
    Event details remain visible for 8 seconds.
    Then the animated name screen returns.

  IMPORTANT:
    Use a suitable driver for the vibration motor.
    Do not connect a motor directly to an Arduino GPIO.
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

const byte pedalPin = 2;
const byte irPin = 3;
const byte flamePin = 4;

const byte irLedPin = 5;
const byte pedalLedPin = 6;
const byte fireLedPin = 7;

const byte motorPin = 8;
const byte buzzerPin = 9;

// =====================================================
// SENSOR POLARITY
// Change a state to HIGH if that sensor activates HIGH.
// =====================================================

const byte PEDAL_ACTIVE_STATE = LOW;
const byte IR_ACTIVE_STATE = LOW;
const byte FLAME_ACTIVE_STATE = LOW;

const bool MOTOR_ACTIVE_HIGH = true;

// =====================================================
// TIMING CONFIGURATION
// =====================================================

const unsigned long MOTOR_ON_DURATION = 500;
const unsigned long EVENT_SCREEN_DURATION = 8000;

const unsigned long IDLE_ANIMATION_INTERVAL = 100;
const unsigned long EVENT_ANIMATION_INTERVAL = 250;

const unsigned long BUZZER_BEEP_DURATION = 70;
const unsigned int BUZZER_BEEP_FREQUENCY = 1400;

const unsigned long SIREN_SWEEP_INTERVAL = 2;

// =====================================================
// SENSOR STATES
// =====================================================

bool pedalPressed = false;
bool irDetected = false;
bool fireDetected = false;

bool previousPedalPressed = false;
bool previousIrDetected = false;
bool previousFireDetected = false;

// =====================================================
// MOTOR CONTROL
// =====================================================

bool motorRunning = false;
unsigned long motorStartMillis = 0;

// =====================================================
// BUZZER CONTROL
// =====================================================

bool sirenRunning = false;
bool beepRunning = false;

unsigned long beepStartMillis = 0;
unsigned long previousSirenMillis = 0;

int currentFrequency = 1000;
int sweepDirection = 15;

// =====================================================
// OLED DISPLAY STATE
// =====================================================

enum EventType {
  EVENT_NONE,
  EVENT_PEDAL,
  EVENT_IR,
  EVENT_FIRE
};

EventType displayedEvent = EVENT_NONE;

bool eventScreenActive = false;

unsigned long eventScreenStartMillis = 0;
unsigned long previousAnimationMillis = 0;

byte idleFrame = 0;
byte eventFrame = 0;

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
// QUIET EVENT BEEP
// =====================================================

void triggerQuietBeep() {

  // A flame event uses the siren instead.
  if (fireDetected || sirenRunning) {
    return;
  }

  beepStartMillis = millis();
  beepRunning = true;

  tone(buzzerPin, BUZZER_BEEP_FREQUENCY);

  Serial.println(F("Quiet event beep"));
}

void updateQuietBeep(unsigned long now) {

  if (beepRunning &&
      now - beepStartMillis >= BUZZER_BEEP_DURATION) {

    beepRunning = false;

    // Do not silence an active fire siren.
    if (!sirenRunning) {
      noTone(buzzerPin);
    }
  }
}

// =====================================================
// READ SENSORS
// =====================================================

void readSensors() {

  pedalPressed =
    digitalRead(pedalPin) == PEDAL_ACTIVE_STATE;

  irDetected =
    digitalRead(irPin) == IR_ACTIVE_STATE;

  fireDetected =
    digitalRead(flamePin) == FLAME_ACTIVE_STATE;
}

// =====================================================
// UPDATE LED INDICATORS
// =====================================================

void updateLeds() {

  digitalWrite(
    pedalLedPin,
    pedalPressed ? HIGH : LOW
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
// DISPLAY EVENT
// =====================================================

void showEvent(EventType event, unsigned long now) {

  displayedEvent = event;

  eventScreenActive = true;
  eventScreenStartMillis = now;

  eventFrame = 0;
  previousAnimationMillis = now;

  Serial.print(F("ALERT: "));

  switch (event) {

    case EVENT_PEDAL:
      Serial.println(F("PEDAL DETECTED"));
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
// PROCESS NEW SENSOR EVENTS
// =====================================================

void processEvents(unsigned long now) {

  bool newPedalEvent =
    pedalPressed && !previousPedalPressed;

  bool newIrEvent =
    irDetected && !previousIrDetected;

  bool newFireEvent =
    fireDetected && !previousFireDetected;

  // Give a newly detected fire event highest priority.
  if (newFireEvent) {

    showEvent(EVENT_FIRE, now);
    triggerVibration();

  } else if (newPedalEvent) {

    showEvent(EVENT_PEDAL, now);
    triggerVibration();

    if (!fireDetected) {
      triggerQuietBeep();
    }

  } else if (newIrEvent) {

    showEvent(EVENT_IR, now);
    triggerVibration();

    if (!fireDetected) {
      triggerQuietBeep();
    }
  }

  previousPedalPressed = pedalPressed;
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

      // Cancel any short event beep before starting siren.
      beepRunning = false;

      currentFrequency = 1000;
      sweepDirection = 15;

      previousSirenMillis = now;

      tone(buzzerPin, currentFrequency);

      Serial.println(F("FIRE SIREN ON"));
    }

    if (now - previousSirenMillis >=
        SIREN_SWEEP_INTERVAL) {

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
// DRAW PEDAL ICON
// =====================================================

void drawPedalIcon(byte frame) {

  // Pedal base
  display.drawRoundRect(
    77, 25, 40, 24, 3, SSD1306_WHITE
  );

  // Pedal plate changes angle during animation.
  if (frame == 0) {

    display.drawLine(
      82, 29, 108, 35, SSD1306_WHITE
    );

    display.drawLine(
      108, 35, 108, 40, SSD1306_WHITE
    );

  } else {

    display.drawLine(
      82, 38, 108, 30, SSD1306_WHITE
    );

    display.drawLine(
      108, 30, 108, 40, SSD1306_WHITE
    );
  }

  // Foot/pedal press indicator
  if (frame == 0) {
    display.drawCircle(96, 18, 4, SSD1306_WHITE);
  } else {
    display.fillCircle(96, 18, 4, SSD1306_WHITE);
  }
}

// =====================================================
// DRAW IR MOTION ICON
// =====================================================

void drawMotionIcon(byte frame) {

  display.drawCircle(100, 35, 4, SSD1306_WHITE);

  if (frame == 0) {

    display.drawCircle(100, 35, 11, SSD1306_WHITE);

  } else {

    display.drawCircle(100, 35, 16, SSD1306_WHITE);
  }
}

// =====================================================
// DRAW FIRE ICON
// =====================================================

void drawFireIcon(byte frame) {

  display.fillTriangle(
    16, 22,
    5, 47,
    27, 47,
    SSD1306_WHITE
  );

  display.fillCircle(
    16, 42, 10, SSD1306_WHITE
  );

  if (frame == 0) {

    display.fillTriangle(
      16, 31,
      11, 44,
      21, 44,
      SSD1306_BLACK
    );

  } else {

    display.fillTriangle(
      16, 28,
      10, 44,
      22, 44,
      SSD1306_BLACK
    );
  }
}

// =====================================================
// DRAW EVENT DETAILS
// =====================================================

void drawEventScreen() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(F("HOME SECURITY ALERT"));

  display.drawLine(
    0, 11, 127, 11, SSD1306_WHITE
  );

  switch (displayedEvent) {

    case EVENT_PEDAL:

      display.setTextSize(2);

      display.setCursor(3, 17);
      display.println(F("PEDAL"));

      display.setCursor(3, 35);
      display.println(F("PRESSED"));

      drawPedalIcon(eventFrame);

      display.setTextSize(1);
      display.setCursor(3, 54);
      display.println(F("Pedal activated"));

      break;

    case EVENT_IR:

      display.setTextSize(2);

      display.setCursor(3, 19);
      display.println(F("MOTION"));

      display.setCursor(3, 38);
      display.println(F("DETECTED"));

      drawMotionIcon(eventFrame);

      break;

    case EVENT_FIRE:

      drawFireIcon(eventFrame);

      display.setTextSize(2);

      display.setCursor(37, 24);
      display.println(F("FIRE"));

      display.setCursor(37, 43);
      display.println(F("ALERT"));

      break;

    default:

      display.setTextSize(1);
      display.setCursor(10, 28);
      display.println(F("SECURITY EVENT"));

      break;
  }

  // Animated warning indicator
  if (eventFrame == 0) {
    display.fillCircle(120, 5, 3, SSD1306_WHITE);
  } else {
    display.drawCircle(120, 5, 3, SSD1306_WHITE);
  }

  display.display();
}

// =====================================================
// ANIMATED IDLE SCREEN
// =====================================================

void drawIdleScreen(byte frame) {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // Moving decorative stars
  int starX1 = (frame * 3) % SCREEN_WIDTH;
  int starX2 = SCREEN_WIDTH - ((frame * 2) % SCREEN_WIDTH);

  display.drawPixel(starX1, 3, SSD1306_WHITE);
  display.drawPixel(starX2, 60, SSD1306_WHITE);

  display.drawPixel(
    (starX1 + 47) % SCREEN_WIDTH,
    8,
    SSD1306_WHITE
  );

  display.drawPixel(
    (starX2 + 31) % SCREEN_WIDTH,
    55,
    SSD1306_WHITE
  );

  // Large name across two lines.
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

  // V-D label
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
// OLED DISPLAY MANAGER
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

  pinMode(pedalPin, INPUT_PULLUP);
  pinMode(irPin, INPUT);
  pinMode(flamePin, INPUT);

  pinMode(irLedPin, OUTPUT);
  pinMode(pedalLedPin, OUTPUT);
  pinMode(fireLedPin, OUTPUT);

  pinMode(motorPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  setMotor(false);
  noTone(buzzerPin);

  Wire.begin();

  Serial.println();
  Serial.println(F("=========================="));
  Serial.println(F("HOME SECURITY SYSTEM"));
  Serial.println(F("=========================="));

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

  // Read initial states without generating startup events.
  readSensors();
  updateLeds();

  previousPedalPressed = pedalPressed;
  previousIrDetected = irDetected;
  previousFireDetected = fireDetected;

  unsigned long now = millis();

  previousAnimationMillis = now;

  Serial.println(F("System ready."));
  Serial.println(F("Pedal sensor: D2"));
  Serial.println(F("IR sensor: D3"));
  Serial.println(F("Flame sensor: D4"));
  Serial.println(F("Vibration: event-triggered"));
  Serial.println(F("Event display: 8 seconds"));
  Serial.println(F("Buzzer: quiet beep / fire siren"));
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

  updateQuietBeep(now);

  updateDisplay(now);
}
