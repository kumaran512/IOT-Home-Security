# Top-Level Testing Checklist

Use the detailed plans in `tests/`. Record setup, part revision, observed result, pass/fail, and corrective action for every test. Do not test with mains wiring exposed.

- [ ] Confirm every module variant and its voltage/interface documentation.
- [ ] Review and approve pin allocation; check ESP32-CAM occupied and boot-sensitive pins.
- [ ] Verify power rails, current budget, polarity, and battery/charger compatibility before connecting controllers.
- [ ] Test each Nano sensor input and local output independently.
- [ ] Verify Nano local alerts continue with ESP32-CAM off and UART disconnected.
- [ ] Test camera/AP independently and confirm no assumed spare GPIO conflicts with camera operation.
- [ ] Test UART levels, framing, malformed input handling, and link-loss behavior.
- [ ] Test integrated power-loss transition and backup runtime against the documented target.
- [ ] Check motor/servo operation for resets, overheating, and supply dips.
- [ ] Review wiring, enclosure, and student-accessible low-voltage safety before demonstration.
