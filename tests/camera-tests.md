# ESP32-CAM Test Plan

Use the exact board and camera combination selected for the project. Check its pin map and supply requirements before attaching peripherals.

1. **Power-up:** Verify stable voltage during camera initialization and Wi-Fi operation; observe resets and heating.
2. **Camera:** Confirm sensor detection and a usable local stream at the selected resolution/frame rate.
3. **Access point:** Confirm the AP starts, a client can connect, and the camera page is reachable on the intended local network.
4. **Recovery:** Test camera initialization failure and Wi-Fi client disconnect/reconnect without affecting Nano local alarm behavior.
5. **UART receive:** Send valid and malformed provisional messages; confirm bounded parsing, no blocking, and sensible diagnostics.
6. **UART transmit/status:** If implemented, verify status output does not collide with programming/debug traffic.
7. **Pin audit:** Confirm camera/flash/PSRAM/boot/programming pins are not reused. Do not connect a servo until a genuinely available signal pin is verified.
8. **Servo (conditional):** With a suitable external supply and mechanical stops, test limited motion and monitor supply stability. Omit if no safe GPIO/power arrangement is available.
9. **Soak:** Run camera and AP for the intended demonstration interval and record disconnects, resets, and temperatures.
