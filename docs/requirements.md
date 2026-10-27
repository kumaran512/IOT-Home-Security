# Requirements

## Functional requirements

- **FR-01 Local monitoring:** The Nano monitors the door, IR disturbance, flame, and main-power status inputs.
- **FR-02 Local alerting:** The Nano indicates status using the three room LEDs and OLED and can activate the vibration motor according to documented alert behavior.
- **FR-03 Independence:** Sensor monitoring and local alerts continue when the ESP32-CAM, Wi-Fi, or UART link is unavailable.
- **FR-04 Camera subsystem:** The ESP32-CAM provides a local Wi-Fi access point and browser camera view, subject to verified module and camera support.
- **FR-05 Event reporting:** The Nano can report defined sensor and status events to the ESP32-CAM over UART. Message format is provisional in `uart-protocol.md`.
- **FR-06 Camera positioning:** Servo positioning is a planned capability, conditional on confirming the servo, mechanical range, GPIO availability, and supply.
- **FR-07 Power status:** Main-power loss is detected through an appropriately isolated/conditioned low-voltage signal; battery backup supports the selected loads.

## Non-functional requirements

- **NFR-01 Safety:** No mains-voltage wiring is placed on a student-accessible breadboard. Mains-side construction, if any, is outside this student-level low-voltage prototype and must be handled using an approved, enclosed supply by a qualified adult.
- **NFR-02 Electrical compatibility:** Nano-to-ESP32 signals must use suitable 5 V-to-3.3 V logic interfacing where the actual Nano output is 5 V and the ESP32 input is not 5 V tolerant. Verify board variants and signal directions.
- **NFR-03 Resilience:** A failure of camera/network functions must not disable local detection or alerts.
- **NFR-04 Power integrity:** Motor and servo loads must not be powered from GPIO pins; supply, driver, grounding, and transient behavior must be validated for actual parts.
- **NFR-05 Maintainability:** Firmware and documentation should use clear names, straightforward control flow, and readable comments suitable for a school-level project.
- **NFR-06 Verifiability:** Pin assignments, thresholds, timing, and acceptance criteria are recorded and tested against the selected hardware before being treated as final.
- **NFR-07 Privacy:** Camera access is limited to the intended local network during the prototype; do not expose the access point or stream to the internet without a separate security review.
