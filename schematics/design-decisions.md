# Design Decisions and Open Questions

## Current preliminary decisions

1. **Local alarm authority:** Nano owns door/IR/flame inputs, room LEDs, motor alert, and the main-power status event. Camera, Wi-Fi, ESP32-CAM, and UART are supplementary; their failure must not disable local monitoring or alerts.
2. **Nano pin proposal:** conditional on a classic ATmega328P Nano pinout: D2 door, D3 IR, D4 flame, D5-D7 LEDs, D8 motor-driver control, A0 main status, A4/A5 OLED I²C, D1/D0 UART TX/RX. This is a proposal for review, not a wiring approval.
3. **ESP32-CAM pins:** no GPIO numbers assigned. Exact board, SoC, camera, flash/PSRAM, boot and serial programming map are unavailable; a safe servo GPIO and UART pair cannot yet be established.
4. **UART:** cross-connect `NANO_TX_5V` to `ESP32_RX_3V3` through a verified 5 V-to-3.3 V shifter; cross-connect `ESP32_TX_3V3` to Nano RX after checking input-high compatibility. Use common signal ground if non-isolated. D0/D1 also share the Nano USB-serial/programming channel on the classic design, so plan isolation/disconnection during upload/debug.
5. **Power path:** use a properly rated dual-source mux/ideal-diode arrangement with reverse-current blocking; do not parallel the source outputs or use the relay module as the power selector. Keep logic and actuator branches named separately. Selection and ratings remain open pending source/load verification.
6. **Motor:** low-side transistor/MOSFET or rated driver controlled by Nano; choose from motor startup/stall current and device losses. Add suppression if required by motor type and driver. Never connect the motor directly to GPIO.
7. **Servo:** power from a verified actuator rail, not a GPIO or undersized controller regulator. Verify signal voltage compatibility and current peaks; common reference required where signal interface is non-isolated.
8. **GPIO inputs and LEDs:** condition each sensor output to the selected MCU's safe logic levels; add individual LED resistors based on actual Vf and target current. No resistor/divider values are invented.

## Assumptions requiring confirmation

- Nano marking ATmega328PU and Mini-USB may be consistent with a classic Nano-compatible board, but do not prove PCB revision, clock, bootloader, logic voltage, pin order, USB bridge, regulator, or input path.
- ESP32-CAM Mini-USB and camera ribbon marking do not identify the board revision, SoC family, sensor, camera pin map, antenna, flash/PSRAM, GPIO availability, or USB-serial wiring.
- OLED GM009605 specs (0.96-inch, 128×64 monochrome, I²C, SSD1306) are user-reported. Verify module pin order, address, pull-up rail, supply/logic levels and current.
- Door, IR and flame output voltage/polarity/topology are not established. Flame VCC/GND/DO labels do not establish supply or output ratings.
- `5 V / 5 A` main source, `5 V / 5000 mAh` USB power bank, and `5 V` relay are user-described ratings only. Verify construction, regulation, battery energy/usable capacity/current, pass-through behavior and protection.
- `Blue SG90` and the two-wire motor require exact identification and datasheets/measurements for voltage and startup/stall current.

## Decisions still needed

- Identify all board/module revisions and obtain primary datasheets/schematics.
- Establish logic and actuator load budgets, including camera/Wi-Fi peaks, servo movement/stall, motor startup/stall, and transient margins.
- Verify sensors' output stages and determine whether pull-ups, dividers, buffers, optoisolation, or Schmitt conditioning are needed.
- Select exact LED resistors, motor driver, suppression, level shifters, power mux, protection, connectors, and rail decoupling from actual electrical requirements.
- Inspect relay circuit; verify active-low threshold, coil supply, isolation, COM/NO/NC, contact rating, and energized/de-energized behavior. Define a main-failure detector separately from relay control polarity.
- Verify USB power-bank minimum load behavior and pass-through. A nominal capacity in mAh is not usable 5 V output energy by itself.
- Confirm Nano upload/debug procedure with UART connected; confirm ESP32 programming UART remains available.
- Verify power-up/reset states and boot-strap behavior with proposed external loads before selecting ESP32 pins.

## Tooling and validation status

No `kicad-cli`, `eeschema`, `drawio`, or `dot` executable was found on `PATH` during this audit. The deliverables are therefore Markdown text schematics and logical netlists, not native KiCad files. No circuit-rule, electrical-rule, simulation, or hardware validation has been performed. File presence does not imply electrical correctness.
