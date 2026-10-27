# Pin Allocation Plan (Provisional)

**No GPIO assignment in this document is approved for wiring.** Select exact boards and modules first, inspect their pinouts and boot constraints, then record measured/verified assignments here. Existing document is a planning worksheet, not a pinout.

| Controller | Signal / function | Candidate pin | Status / verification needed |
|---|---|---|---|
| Nano | Magnetic door sensor | TBD | Confirm sensor contact type, pull-up/pull-down, and desired fail-safe state |
| Nano | IR disturbance sensor | TBD | Confirm module output voltage, polarity, and output type |
| Nano | Flame sensor | TBD | Confirm module and whether digital or analog output is needed; assess false-trigger behavior |
| Nano | OLED interface | TBD | Confirm OLED controller, I²C/SPI interface, voltage, and address if I²C |
| Nano | Three room LEDs | TBD | Choose pins; include current-limiting resistors and confirm aggregate current |
| Nano | Vibration driver control | TBD | GPIO drives driver input only, never motor directly |
| Nano | Main-power status input | TBD | Confirm low-voltage isolated/conditioned detector output and fail-safe logic |
| Nano ↔ ESP32-CAM | UART TX/RX | TBD | Cross-connect TX/RX; verify Nano logic level and add suitable 5 V-to-3.3 V interface on ESP32 RX path as required |
| ESP32-CAM | Camera | Board-defined | Verify exact board/camera pin map; do not reassign camera pins |
| ESP32-CAM | UART | TBD | Check board variant, camera configuration, boot/strapping pins, and serial programming/debug usage |
| ESP32-CAM | Servo signal | TBD / possibly unavailable | Confirm a safe spare GPIO after camera configuration; do not assume one exists |

## Review procedure

1. Record board manufacturer/model/revision, MCU, operating voltage, and pinout source for each controller.
2. Record each module's supply range, output levels, interface, and active polarity from its datasheet or direct measurement.
3. For the ESP32-CAM, identify the exact module and camera configuration and mark every occupied, boot-strapping, flash/PSRAM, and programming pin before considering spare pins.
4. Check Nano pin conflicts, timer/PWM needs, serial programming, and library requirements.
5. Update this table with pin names and rationale; independently review before wiring. Keep wiring changes synchronized with `hardware/connections.md`.
