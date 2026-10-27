# Component Inventory

This is the project-level inventory. The user-provided markings and OLED details below have not been independently verified; electrical ratings and exact board variants remain pending. Use [the hardware verification checklist](verification-checklist.md) as the detailed record before wiring or allocating GPIOs.

| Component | Intended role | Details to record before wiring |
|---|---|---|
| Arduino Nano | Primary sensor/alert controller | User-observed: ATmega328PU marking and Mini-USB connector. Verify manufacturer/revision, clock, bootloader, logic voltage, input limits and regulator/current limits. |
| ESP32-CAM and camera | Independent camera and Wi-Fi AP | User-observed Mini-USB connector and camera ribbon marking TY-OV2 640-V2.0. Verify board revision, camera sensor, flash/PSRAM, antenna, GPIO map and supply requirements. |
| 0.96-inch OLED | Local status | User-reported marking GM009605, 128 × 64 monochrome, I²C, SSD1306 controller. Verify module details, exact pin order, supply/logic levels and current. |
| Magnetic door sensor | Door state | User-reported magnetic reed switch. Verify NO/NC behavior, electrical limits, cable and pull-up needs. |
| IR disturbance sensor | Disturbance input | User-reported binary-output sensor. Verify exact module, output voltage/polarity, electrical limits, range and adjustment. |
| Flame sensor | Flame/hazard input | User-reported module with VCC, GND, DO terminals and binary output. Verify module, output voltage/polarity, electrical limits, sensitivity and limitations. |
| Three LEDs | Room/status indicators | Color, forward voltage/current, resistor values and indicator mapping |
| Vibration motor | Local alert | User-reported two-wire motor. Verify model, rated/stall current, voltage, driver and suppression needs. |
| Servo | Planned camera positioning | User-reported blue SG90. Verify exact variant, voltage, moving/stall current, torque and travel limits. |
| Main supply | Low-voltage input | User-described DIY 5 V, 5 A supply intended for the setup. Verify topology, regulation, protection, output tolerance/current, connector and polarity. |
| Battery, charger and power path | Backup | User-described DIY 5 V battery, 5 V 5 A charger, and single-channel 5 V relay module. Verify battery chemistry/capacity/protection, charger type/compatibility, relay input/contact details, and safe power-path/changeover design. |
| Regulators, drivers and level interface | Conditioning | Part/model, input/output range, current, logic thresholds and thermal/transient limits |

Every electrical value remains TBD until supported by a part marking, datasheet, or documented measurement. Preserve the source and measured results with the project before finalizing the connection notes.
