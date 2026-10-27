/*
 * ESP32-CAM entry point — planning skeleton only.
 * Verify the exact ESP32-CAM board, camera pin configuration, and supply
 * before selecting peripherals. Do not assume spare GPIOs for a servo.
 * See docs/pin-allocation.md and docs/power-management.md.
 */

void setup() {
  // TODO: initialize the verified camera and local access point.
  // Camera/network startup must not be a prerequisite for Nano local alarms.
}

void loop() {
  // TODO: service camera/network and bounded, non-blocking UART reception.
}
