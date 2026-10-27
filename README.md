# IoT Based Home Security System

A school-level IoT home security prototype combining local sensor monitoring, hazard alerts, battery backup, and wireless camera surveillance. The Arduino Nano is the primary local controller; the ESP32-CAM is an independent camera and Wi-Fi access-point subsystem.

## Hardware overview

Before wiring, identify the actual boards and components in the [hardware verification checklist](hardware/verification-checklist.md).

- Arduino Nano (exact board / logic-voltage variant to be confirmed)
- ESP32-CAM (exact module and camera configuration to be confirmed)
- 0.96-inch OLED display (controller, interface, and voltage to be confirmed)
- Three room-indicator LEDs
- Magnetic door sensor, IR disturbance sensor, and flame sensor
- Vibration motor for local alerts
- Servo motor for camera positioning
- Main-power failure detection and battery backup

## Architecture

The Nano monitors the local sensors and drives local indicators and alerts. These functions must continue if the ESP32-CAM, UART link, or Wi-Fi is unavailable. The ESP32-CAM provides a browser-accessible camera over its own access point and may receive event messages from the Nano. Camera operation and network availability are supplementary to local alarm behavior.

See [system architecture](docs/system-architecture.md) and [UART protocol plan](docs/uart-protocol.md).

## Repository structure

```text
docs/                  Project, requirements, architecture, wiring, and plans
firmware/arduino-nano/ Arduino Nano firmware entry point
firmware/esp32-cam/    ESP32-CAM firmware entry point
hardware/              Component inventory and connection notes
tests/                 Subsystem and integration test plans
```

## Development roadmap

1. Identify exact board, sensor, display, servo, motor-driver, and battery variants.
2. Review the provisional pin-allocation and UART plans against those parts.
3. Build and check low-voltage circuits and power rails; validate sensor behavior.
4. Implement and test Nano local monitoring and alert behavior.
5. Implement and test the ESP32-CAM access point and camera independently.
6. Integrate over a level-safe UART link and execute the test plans.

No full firmware is implemented yet. Pin assignments and electrical details remain unconfirmed; consult the [pin plan](docs/pin-allocation.md), [power plan](docs/power-management.md), and [wiring guide](docs/wiring-guide.md) before connecting hardware. Do not place mains-voltage wiring on a student-accessible breadboard.
