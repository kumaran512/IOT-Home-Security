# IoT Based Home Security System

A school-level IoT home security prototype combining local sensor monitoring, hazard alerts, battery backup, and wireless camera surveillance.

## Hardware

- Arduino Nano
- ESP32-CAM
- 0.96-inch OLED display
- Three room-indicator LEDs
- Magnetic door sensor
- IR disturbance sensor
- Flame sensor
- Vibration motor
- Servo motor for camera positioning
- Main power-failure detection and battery backup circuit

## Architecture

### Arduino Nano
- Monitor door, IR, and flame sensors.
- Control the three room LEDs and OLED display.
- Operate the vibration alert.
- Monitor main power availability.
- Send events to the ESP32-CAM over UART.

### ESP32-CAM
- Create a dedicated Wi-Fi access point.
- Provide a browser-based live camera view.
- Control the camera's servo position.
- Receive sensor-event messages from the Nano.

## Development Principles

- Local alerts must work independently of Wi-Fi and the camera.
- Confirm all pin assignments against the actual hardware before wiring.
- Protect ESP32 inputs from 5 V logic signals.
- Use suitable drivers and regulated supplies for motors.
- Design battery backup around the actual battery chemistry and load requirements.

## Project Status

Initial repository setup. Hardware verification, pin allocation, firmware development, and integration testing are pending.
