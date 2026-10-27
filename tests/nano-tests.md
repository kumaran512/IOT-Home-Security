# Arduino Nano Test Plan

Run on the selected Nano variant with the camera link disconnected first. Record firmware revision, supply voltage, sensor variants, and results.

1. **Power-up:** Verify regulated input and stable startup; confirm no motor/servo current is drawn through a GPIO.
2. **Door input:** Exercise open/closed states repeatedly; verify polarity, debounce, and expected LED/OLED/alert behavior once defined.
3. **IR input:** Trigger and clear the sensor; verify active polarity and that noise or sustained input is handled as specified.
4. **Flame input:** Safely use a simulated/electrical sensor stimulus where practical; do not create an uncontrolled fire hazard. Confirm threshold/polarity handling.
5. **Indicators:** Verify each of the three LEDs independently and check resistor/current choices.
6. **OLED:** Verify startup, readable status, and updates for sensor and power states.
7. **Vibration output:** Test through its driver with the intended supply; inspect voltage dip, heating, and unintended resets.
8. **Power-failure input:** Simulate the detector's low-voltage output; verify state reporting and local operation on backup.
9. **Independence:** Repeat sensor and alert checks with ESP32-CAM powered off and UART disconnected. Nano must not hang or lose local behavior.
10. **Endurance:** Exercise repeated sensor transitions and alerts for the planned demonstration interval; log missed events or resets.
