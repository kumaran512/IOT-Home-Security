# Wiring Guide (Planning Stage)

**Do not wire from this guide until part variants and the pin-allocation table are verified.** It describes connection rules, not a completed pin-by-pin schematic.

1. Identify exact Nano, ESP32-CAM, camera, OLED, sensor modules, motor, servo, regulator, and battery/charger models. Check their datasheets and board pinouts.
2. Establish regulated low-voltage rails and a current budget first. Keep motor/servo current off controller GPIO and separate their power/return path as appropriate. Join grounds for signal references where required.
3. Connect each sensor only after confirming supply range, output type/voltage, active polarity, and required pull-up/pull-down or conditioning. Do not feed an unverified sensor output directly to a controller input.
4. Connect the OLED only after confirming interface, address (if applicable), and logic/supply compatibility. Add LED current-limiting resistors.
5. Connect the vibration motor through a suitable driver. Connect the servo signal only to a verified available output; power the servo from a supply sized for its stall current.
6. For UART, cross-connect Nano TX to ESP32 RX and ESP32 TX to Nano RX, with common signal reference where required. Apply suitable level interfacing for any 5 V Nano TX into a 3.3 V ESP32 input. Verify the reverse direction meets the Nano input threshold.
7. Route main-power failure detection through an isolated/conditioned low-voltage output. Keep mains voltage entirely out of student-accessible breadboards and exposed prototype wiring; use an enclosed certified adapter for mains conversion.
8. Inspect for shorts and polarity errors, measure rails before inserting controllers, then bring up one subsystem at a time using current-limited low-voltage supplies.

Record the final wiring and pin names in `hardware/connections.md`. A schematic and enclosure plan should be added after hardware choices are confirmed.
