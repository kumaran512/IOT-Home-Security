const int buzzerPin = 8; // Connect piezo buzzer positive wire to pin 8

// Timing variables for non-blocking execution
unsigned long previousMillis = 0;
const long sweepInterval = 3; // Controls the speed of the pitch change (lower is faster)

int currentFrequency = 1000;  // Starting frequency in Hz
int sweepDirection = 15;      // How much the pitch jumps per step

void setup() {
  pinMode(buzzerPin, OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  // Update the siren sound at regular intervals
  if (currentMillis - previousMillis >= sweepInterval) {
    previousMillis = currentMillis;

    // Shift the pitch
    currentFrequency += sweepDirection;

    // Bounce the pitch back down/up at fire alarm thresholds (1000Hz - 2200Hz)
    if (currentFrequency >= 2200 || currentFrequency <= 1000) {
      sweepDirection = -sweepDirection; // Reverse the frequency sweep
    }

    // Output the sharp, continuous siren tone
    tone(buzzerPin, currentFrequency);
  }
}
