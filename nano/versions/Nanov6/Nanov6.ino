/*
  IoT Based Home Security System
  Controller: Arduino Nano (ATmega328P)

  Features:
  - Animated OLED idle screen
  - Pedal sensor detection
  - IR motion detection
  - Flame sensor detection
  - Three short beeps for pedal and IR events
  - Sweeping siren for fire detection
  - Vibration motor alert
  - Event details displayed for 8 seconds
  - Non-blocking timing using millis()

  OLED: SSD1306 128x64, I2C address 0x3C

  IMPORTANT:
  - Drive the vibration motor through a suitable transistor/driver.
  - Do not connect a motor directly to an Arduino GPIO.
  - Ensure the buzzer type supports tone() output.
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// OLED CONFIGURATION
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// PIN CONFIGURATION
// =====================================================

const byte PEDAL_PIN       = 2;
const byte IR_SENSOR_PIN   = 3;
const byte FLAME_SENSOR_PIN = 4;

const byte IR_LED_PIN      = 5;
const byte PEDAL_LED_PIN   = 6;
const byte FIRE_LED_PIN    = 7;

const byte MOTOR_PIN       = 8;
const byte BUZZER_PIN      = 9;

// =====================================================
// SENSOR ACTIVE STATES
// Change these to HIGH if your sensors work oppositely.
// =====================================================

const byte PEDAL_ACTIVE_STATE = LOW;
const byte IR_ACTIVE_STATE    = LOW;
const byte FLAME_ACTIVE_STATE = LOW;

// =====================================================
// TIMING CONFIGURATION
// =====================================================

const unsigned long EVENT_SCREEN_DURATION = 8000UL;

const unsigned long MOTOR_ON_DURATION = 500UL;

const unsigned long BUZZER_BEEP_DURATION = 70UL;
const unsigned long BUZZER_GAP_DURATION  = 100UL;

const int BUZZER_BEEP_FREQUENCY = 1400;
const byte BUZZER_BEEP_COUNT = 3;

const unsigned long SIREN_UPDATE_INTERVAL = 8UL;
const unsigned long IDLE_ANIMATION_INTERVAL = 180UL;

// =====================================================
// SENSOR STATES
// =====================================================

bool pedalDetected = false;
bool irDetected = false;
bool fireDetected = false;

bool previousPedalDetected = false;
bool previousIRDetected = false;
bool previousFireDetected = false;

// =====================================================
// EVENT DISPLAY
// =====================================================

enum EventType {
  EVENT_NONE,
  EVENT_PEDAL,
  EVENT_MOTION,
  EVENT_FIRE
};

EventType currentEvent = EVENT_NONE;

unsigned long eventDisplayStart = 0;

// =====================================================
// VIBRATION MOTOR
// =====================================================

bool motorActive = false;
unsigned long motorStartTime = 0;

// =====================================================
// THREE-BEEP SEQUENCE
// =====================================================

bool beepActive = false;
bool buzzerToneOn = false;

byte beepCount = 0;
unsigned long beepTimer = 0;

// =====================================================
// FIRE SIREN
// =====================================================

bool sirenActive = false;

int sirenFrequency = 1000;
int sirenDirection = 1;

unsigned long lastSirenUpdate = 0;

// =====================================================
// IDLE ANIMATION
// =====================================================

unsigned long lastAnimationUpdate = 0;

int eyeOffset = 0;
int eyeDirection = 1;

byte animationFrame = 0;

// =====================================================
// FUNCTION DECLARATIONS
// =====================================================

void readSensors();
void processEvents();

void triggerQuietBeep();
void updateQuietBeep();

void updateSiren();
void startMotor();
void updateMotor();

void showIdleScreen();
void showEventScreen(EventType eventType);

void drawIdleEyes();
void drawPedalIcon();
void drawMotionIcon();
void drawFireIcon();

void updateDisplay();

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(9600);

  pinMode(PEDAL_PIN, INPUT_PULLUP);
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

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("OLED initialization failed."));
  } else {
    Serial.println(F("OLED initialization successful."));

    display.clearDisplay();
    display.display();

    showIdleScreen();
  }

  readSensors();

  previousPedalDetected = pedalDetected;
  previousIRDetected = irDetected;
  previousFireDetected = fireDetected;

  Serial.println(F("Home Security System Ready"));
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop() {
  readSensors();

  processEvents();

  updateMotor();

  updateSiren();

  updateQuietBeep();

  updateDisplay();
}

// =====================================================
// READ SENSORS
// =====================================================

void readSensors() {
  pedalDetected =
    (digitalRead(PEDAL_PIN) == PEDAL_ACTIVE_STATE);

  irDetected =
    (digitalRead(IR_SENSOR_PIN) == IR_ACTIVE_STATE);

  fireDetected =
    (digitalRead(FLAME_SENSOR_PIN) == FLAME_ACTIVE_STATE);

  digitalWrite(
    PEDAL_LED_PIN,
    pedalDetected ? HIGH : LOW
  );

  digitalWrite(
    IR_LED_PIN,
    irDetected ? HIGH : LOW
  );

  digitalWrite(
    FIRE_LED_PIN,
    fireDetected ? HIGH : LOW
  );
}

// =====================================================
// PROCESS NEW SENSOR EVENTS
// =====================================================

void processEvents() {
  bool newPedalEvent =
    pedalDetected && !previousPedalDetected;

  bool newIREvent =
    irDetected && !previousIRDetected;

  bool newFireEvent =
    fireDetected && !previousFireDetected;

  // Fire has priority when multiple events begin together.
  if (newFireEvent) {
    currentEvent = EVENT_FIRE;
    eventDisplayStart = millis();

    startMotor();

    Serial.println(F("ALERT: FIRE DETECTED"));
  }
  else if (newPedalEvent) {
    currentEvent = EVENT_PEDAL;
    eventDisplayStart = millis();

    startMotor();

    if (!fireDetected) {
      triggerQuietBeep();
    }

    Serial.println(F("ALERT: PEDAL ACTIVATED"));
  }
  else if (newIREvent) {
    currentEvent = EVENT_MOTION;
    eventDisplayStart = millis();

    startMotor();

    if (!fireDetected) {
      triggerQuietBeep();
    }

    Serial.println(F("ALERT: MOTION DETECTED"));
  }

  previousPedalDetected = pedalDetected;
  previousIRDetected = irDetected;
  previousFireDetected = fireDetected;
}

// =====================================================
// START THREE SHORT BEEPS
// =====================================================

void triggerQuietBeep() {
  // Do not start ordinary beeps during a fire alarm.
  if (fireDetected || sirenActive) {
    return;
  }

  beepCount = 0;
  beepActive = true;
  buzzerToneOn = false;

  // Start the sequence immediately.
  beepTimer = millis() - BUZZER_GAP_DURATION;
}

// =====================================================
// UPDATE THREE-BEEP SEQUENCE
// Non-blocking: no delay() calls.
// =====================================================

void updateQuietBeep() {
  if (!beepActive) {
    return;
  }

  // Fire siren takes priority over ordinary beeps.
  if (fireDetected || sirenActive) {
    beepActive = false;
    buzzerToneOn = false;
    noTone(BUZZER_PIN);
    return;
  }

  unsigned long now = millis();

  unsigned long requiredInterval =
    buzzerToneOn
      ? BUZZER_BEEP_DURATION
      : BUZZER_GAP_DURATION;

  if (now - beepTimer < requiredInterval) {
    return;
  }

  beepTimer = now;

  if (buzzerToneOn) {
    // End the current beep.
    noTone(BUZZER_PIN);

    buzzerToneOn = false;
    beepCount++;

    if (beepCount >= BUZZER_BEEP_COUNT) {
      beepActive = false;
    }
  }
  else {
    // Start the next beep.
    if (beepCount < BUZZER_BEEP_COUNT) {
      tone(BUZZER_PIN, BUZZER_BEEP_FREQUENCY);
      buzzerToneOn = true;
    }
  }
}

// =====================================================
// FIRE SIREN
// Sweeps from 1000 Hz to 2200 Hz and back.
// =====================================================

void updateSiren() {
  if (fireDetected) {
    if (!sirenActive) {
      sirenActive = true;

      beepActive = false;
      buzzerToneOn = false;

      sirenFrequency = 1000;
      sirenDirection = 1;

      tone(BUZZER_PIN, sirenFrequency);

      Serial.println(F("FIRE SIREN ACTIVATED"));
    }

    unsigned long now = millis();

    if (now - lastSirenUpdate >= SIREN_UPDATE_INTERVAL) {
      lastSirenUpdate = now;

      sirenFrequency += 20 * sirenDirection;

      if (sirenFrequency >= 2200) {
        sirenFrequency = 2200;
        sirenDirection = -1;
      }

      if (sirenFrequency <= 1000) {
        sirenFrequency = 1000;
        sirenDirection = 1;
      }

      tone(BUZZER_PIN, sirenFrequency);
    }
  }
  else {
    if (sirenActive) {
      sirenActive = false;

      noTone(BUZZER_PIN);

      Serial.println(F("FIRE SIREN DEACTIVATED"));
    }
  }
}

// =====================================================
// VIBRATION MOTOR
// =====================================================

void startMotor() {
  digitalWrite(MOTOR_PIN, HIGH);

  motorActive = true;
  motorStartTime = millis();
}

void updateMotor() {
  if (!motorActive) {
    return;
  }

  if (millis() - motorStartTime >= MOTOR_ON_DURATION) {
    digitalWrite(MOTOR_PIN, LOW);
    motorActive = false;
  }
}

// =====================================================
// DISPLAY UPDATE
// =====================================================

void updateDisplay() {
  unsigned long now = millis();

  if (currentEvent != EVENT_NONE) {
    if (now - eventDisplayStart < EVENT_SCREEN_DURATION) {
      showEventScreen(currentEvent);
      return;
    }

    currentEvent = EVENT_NONE;
    showIdleScreen();
    return;
  }

  if (now - lastAnimationUpdate >= IDLE_ANIMATION_INTERVAL) {
    lastAnimationUpdate = now;

    eyeOffset += eyeDirection;

    if (eyeOffset >= 4) {
      eyeDirection = -1;
    }

    if (eyeOffset <= -4) {
      eyeDirection = 1;
      animationFrame++;
    }

    showIdleScreen();
  }
}

// =====================================================
// IDLE SCREEN
// =====================================================

void showIdleScreen() {
  display.clearDisplay();

  // Animated eyes.
  drawIdleEyes();

  // Name.
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 30);
  display.println(F("Dharsshith"));

  display.setCursor(0, 47);
  display.println(F("Kumaran"));

  // Version label.
  display.setTextSize(1);
  display.setCursor(99, 0);
  display.println(F("V-D"));

  display.display();
}

// =====================================================
// ANIMATED IDLE EYES
// =====================================================

void drawIdleEyes() {
  int leftX = 36;
  int rightX = 88;

  int eyeY = 12;

  int pupilOffset = eyeOffset;

  // Eye outlines.
  display.drawRoundRect(
    leftX - 12,
    eyeY - 8,
    24,
    17,
    5,
    SSD1306_WHITE
  );

  display.drawRoundRect(
    rightX - 12,
    eyeY - 8,
    24,
    17,
    5,
    SSD1306_WHITE
  );

  // Pupils.
  display.fillCircle(
    leftX + pupilOffset,
    eyeY,
    4,
    SSD1306_WHITE
  );

  display.fillCircle(
    rightX + pupilOffset,
    eyeY,
    4,
    SSD1306_WHITE
  );

  // Occasional blink.
  if (animationFrame % 8 == 7) {
    display.drawFastHLine(
      leftX - 10,
      eyeY,
      20,
      SSD1306_BLACK
    );

    display.drawFastHLine(
      rightX - 10,
      eyeY,
      20,
      SSD1306_BLACK
    );
  }
}

// =====================================================
// EVENT SCREEN
// =====================================================

void showEventScreen(EventType eventType) {
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  switch (eventType) {
    case EVENT_PEDAL:
      drawPedalIcon();

      display.setTextSize(2);
      display.setCursor(30, 0);
      display.println(F("PEDAL"));

      display.setTextSize(1);
      display.setCursor(0, 43);
      display.println(F("Pedal activated"));

      display.setCursor(0, 54);
      display.println(F("Security alert"));

      break;

    case EVENT_MOTION:
      drawMotionIcon();

      display.setTextSize(2);
      display.setCursor(30, 0);
      display.println(F("MOTION"));

      display.setTextSize(1);
      display.setCursor(0, 43);
      display.println(F("Movement detected"));

      display.setCursor(0, 54);
      display.println(F("Security alert"));

      break;

    case EVENT_FIRE:
      drawFireIcon();

      display.setTextSize(2);
      display.setCursor(30, 0);
      display.println(F("FIRE!"));

      display.setTextSize(1);
      display.setCursor(0, 43);
      display.println(F("Flame detected"));

      display.setCursor(0, 54);
      display.println(F("EVACUATE SAFELY"));

      break;

    default:
      break;
  }

  display.display();
}

// =====================================================
// PEDAL ICON
// =====================================================

void drawPedalIcon() {
  // Foot/pedal symbol.
  display.drawRoundRect(
    43, 22, 42, 14, 3, SSD1306_WHITE
  );

  display.fillRect(
    48, 25, 32, 5, SSD1306_WHITE
  );

  display.drawLine(
    64, 12, 64, 21, SSD1306_WHITE
  );

  display.fillTriangle(
    59, 17,
    69, 17,
    64, 23,
    SSD1306_WHITE
  );
}

// =====================================================
// MOTION ICON
// =====================================================

void drawMotionIcon() {
  // Head
  display.drawCircle(64, 18, 6, SSD1306_WHITE);

  // Body
  display.drawLine(64, 24, 64, 35, SSD1306_WHITE);

  // Arms
  display.drawLine(64, 27, 53, 33, SSD1306_WHITE);
  display.drawLine(64, 27, 75, 33, SSD1306_WHITE);

  // Legs
  display.drawLine(64, 35, 56, 44, SSD1306_WHITE);
  display.drawLine(64, 35, 72, 44, SSD1306_WHITE);

  // Motion indicators on the left
  display.drawLine(42, 18, 49, 18, SSD1306_WHITE);
  display.drawLine(39, 24, 47, 27, SSD1306_WHITE);
  display.drawLine(42, 32, 49, 30, SSD1306_WHITE);

  // Motion indicators on the right
  display.drawLine(79, 18, 86, 18, SSD1306_WHITE);
  display.drawLine(81, 27, 89, 24, SSD1306_WHITE);
  display.drawLine(79, 30, 86, 32, SSD1306_WHITE);
}
// =====================================================
// FIRE ICON
// =====================================================

void drawFireIcon() {
  display.drawTriangle(
    64, 8,
    47, 35,
    81, 35,
    SSD1306_WHITE
  );

  display.fillTriangle(
    64, 17,
    54, 34,
    74, 34,
    SSD1306_WHITE
  );

  display.fillTriangle(
    64, 25,
    59, 34,
    69, 34,
    SSD1306_BLACK
  );
}
