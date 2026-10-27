# ESP32-CAM Connections (Preliminary)

The camera ribbon is marked `TY-OV2 640-V2.0` and the board has a user-reported Mini-USB connector. These markings do not identify the ESP32-CAM board revision, SoC variant, camera sensor, flash/PSRAM configuration, or pin map. **No physical ESP32 GPIO numbers are assigned in this proposal.** Identify the board and obtain its exact schematic/pinout before selecting UART or servo pins.

| Signal | Proposed connection | Electrical/interface requirement |
|---|---|---|
| `CAMERA_IF` | Board's camera connector/ribbon | Preserve board-defined camera mapping. Identify the sensor and connector orientation before handling/connecting the ribbon. |
| `ESP32_RX_3V3` | From `NANO_TX_5V` through a unidirectional 5 V-to-3.3 V interface | Verify the interface input/output thresholds, edge rate at selected baud, and ESP32 pin assignment. No direct 5 V into ESP32 input. |
| `ESP32_TX_3V3` | To `NANO_RX` through verified compatible path | Confirm Nano input-high threshold and ESP32 output level; condition/translate if needed. |
| `SERVO_PWM` | From a verified available output via a compatible signal interface | Select GPIO only after board/camera/flash/PSRAM/boot/programming audit. Verify SG90 signal level and timing against its actual documentation. |
| `SERVO_V+` / `SERVO_GND` | Separate, adequately sized actuator branch | Do not power servo from GPIO or assume the ESP32-CAM/Nano regulator can supply stall current. Common signal ground with controller when non-isolated. |
| `5V_CAM` / `GND_LOGIC` | Selected system power rail through verified board input | Confirm whether Mini-USB, 5V pin, or another input is intended; do not feed multiple inputs concurrently absent board guidance. |

## Pin resource review required before assignment

The project files do not establish whether the board is an AI-Thinker-style ESP32-CAM, another ESP32-CAM design, or a later SoC/board variant. In particular:

- Map every camera signal and any flash/PSRAM use from the exact board schematic.
- Identify boot-strapping pins and required reset-time levels; avoid servo/UART loading that changes boot behavior.
- Identify programming/debug UART pins and whether Mini-USB circuitry is connected to them. A UART allocation must not prevent flashing or boot logs without a planned isolation arrangement.
- Check the exact SoC datasheet for GPIO voltage limits and input-only pins. Espressif's ESP32 guidance warns to respect strapping-pin states, avoid flash/PSRAM pins, and notes GPIO34+ are input-only on the original ESP32 ([hardware design guidance](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/esp-hardware-design-guidelines.html), [GPIO API](https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/api-reference/peripherals/gpio.html)). This applies only if the board actually uses that ESP32 family/variant.
- Determine whether at least one suitable UART RX/TX pair and one servo-capable output remain after camera and programming use. If not, redesign the interface; do not borrow camera or boot pins.

Until that audit is complete, `ESP32_RX_3V3`, `ESP32_TX_3V3`, and `SERVO_PWM` are **logical net names only**, not board pin assignments. The camera/AP subsystem must remain supplemental to Nano local alerts.
