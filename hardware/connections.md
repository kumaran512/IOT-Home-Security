# Connection Notes (Unverified)

No physical wiring has been validated. This page tracks constraints that must be respected when a pin-level schematic is prepared.

- The Nano is the local control and alarm path; sensor and alert wiring must not depend on the ESP32-CAM.
- Use signal conditioning appropriate to each sensor's output. Confirm active level and input voltage before connection.
- Add current-limiting resistors to LEDs. Drive the vibration motor through a suitable transistor/MOSFET or driver, with suppression as required by the motor/driver design.
- Power a servo from a supply sized for its stall current, not from controller GPIO or an undersized board regulator.
- Check common-ground and return-current arrangement for each separately powered load.
- UART TX/RX must be cross-connected and voltage-compatible. Protect ESP32-CAM inputs from 5 V Nano outputs with suitable 5 V-to-3.3 V logic interfacing. Verify the actual Nano variant and ESP32 input specifications.
- ESP32-CAM GPIO availability depends on exact module and camera configuration. No servo pin is allocated here; account for camera, flash, PSRAM, boot, and serial programming pins.
- Main-power-failure sensing must use an isolated/conditioned low-voltage signal. No mains conductors on student-accessible breadboards.

## Power-failure detector test safety

Test only through a documented low-voltage detector input or by disconnecting the certified adapter's low-voltage output at its connector/switch. Keep the adapter closed; do not probe, switch, or expose mains wiring. Use current-limited isolated low-voltage test equipment only when the detector design specifies that method. Verify the permitted range and polarity first, measure only documented low-voltage points, and do not bypass battery protection or the designed power path. See the step-by-step procedure in `verification-checklist.md`.

**To complete:** add component references and exact pin names only after review of `docs/pin-allocation.md`; attach a schematic and photographs of the final low-voltage build.
