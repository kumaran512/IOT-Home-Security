# System Architecture

## Subsystems

```text
Door / IR / flame / power-status sensors
                  │ (conditioned inputs; exact interfaces TBD)
                  ▼
       Arduino Nano — primary local controller
        ├── OLED and three room-indicator LEDs
        ├── vibration motor through a suitable driver
        ├── local alert and sensor state machine
        └── UART (level-safe interface) ──────┐
                                              ▼
                                  ESP32-CAM subsystem
                                  ├── camera and local AP
                                  └── optional servo control

Low-voltage power system: regulated rails, backup, and power-status signal
```

This is a logical diagram, not a wiring diagram. The ESP32-CAM module, camera, and board pinout vary. Do not infer spare GPIOs from the diagram or assume a particular Nano/OLED/sensor interface.

## Responsibilities and failure boundaries

- The Nano owns sensor sampling, alarm decisions, local indicators, vibration alert, and power-status response.
- The ESP32-CAM owns camera startup and local Wi-Fi access-point/live-view functions. Its failure must not block Nano tasks.
- UART is an optional reporting path. The Nano must not wait indefinitely for acknowledgements or camera readiness.
- Servo operation belongs to the camera subsystem only if the actual board leaves a suitable signal pin free after accounting for camera, flash/PSRAM, boot strapping, and serial programming needs. Otherwise redesign the interface or omit the servo.
- The power system must provide a stable logic supply and separately address motor/servo current. Common reference/grounding is required for signal interfaces where the selected power topology calls for it.

## Startup and degraded operation

Start local Nano monitoring and alerts without waiting for the ESP32-CAM. Attempt serial reporting opportunistically. If no camera status is available, continue local behavior and indicate the unavailable subsystem if a display/LED convention is later defined. Exact timeout and indication behavior remain to be specified during implementation.
