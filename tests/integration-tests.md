# Integrated System Test Plan

Complete subsystem tests first. Record exact board revisions, wiring revision, software revision, supply/battery state, and each outcome.

1. **UART electrical check:** With power off, inspect TX/RX routing and ground reference. Before linking, measure idle levels and confirm level interfacing protects 3.3 V inputs.
2. **Protocol exchange:** Send each defined event and verify the ESP32-CAM receives complete lines; test partial, malformed, oversized, duplicate, and missing-sequence messages.
3. **Local autonomy:** Trigger each sensor with the ESP32-CAM off, Wi-Fi disabled, and UART disconnected in turn. Verify Nano sensing and local alerts continue.
4. **Camera/AP operation:** With Nano active, verify camera stream and AP remain usable while Nano sensor events are reported.
5. **Simultaneous load:** Trigger alerts while camera/Wi-Fi is active; monitor rails for dips, resets, or corrupted readings.
6. **Main power loss:** Simulate loss through the approved low-voltage detector and verify safe switchover, defined load priorities, and no unexpected reset. Do not manipulate exposed mains wiring.
7. **Backup runtime:** Measure against a runtime target selected from the documented power budget; note which loads are shed.
8. **Servo interaction (conditional):** Test camera positioning while streaming and verify it does not disrupt logic power or camera pin functions.
9. **Demonstration readiness:** Inspect enclosure, strain relief, low-voltage-only student-accessible wiring, battery protection, and recovery behavior.
