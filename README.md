# IoT-Based Home Security System

An IoT home-security prototype combining local sensor monitoring, hazard alerts, power-failure backup, and wireless camera surveillance.

The system uses two controllers:

- **Arduino Nano:** local sensor monitoring, room indicators, and alerts.
- **ESP32-CAM:** camera and Wi-Fi access-point functionality.

Local safety functions should remain independent of the camera, Wi-Fi, and UART connection.

## Firmware

### Arduino Nano

The confirmed final sketch is:

`nano/Nanov12/Nanov12.ino`

Earlier Nano sketches are preserved under `nano/versions/`. See [Nano firmware notes](nano/README.md).

### ESP32-CAM

The active development source is:

`esp32cam-test/src/main.cpp`

The PlatformIO configuration is in `esp32cam-test/platformio.ini`.

Prebuilt release binaries are in `esp32cam-test/firmware-release/`. Historical firmware ZIP archives are in `esp32cam-test/firmware-archives/`.

Historical source snapshots are retained under `esp32cam-test/src/`. These snapshots are not necessarily the current firmware.

## Repository layout

- `nano/` — final Nano sketch and historical versions.
- `esp32cam-test/` — ESP32-CAM source, PlatformIO configuration, release binaries, and historical firmware archives.
- `docs/` — system architecture, requirements, pin allocation, wiring, UART protocol, power management, and testing checklist.
- `hardware/` — component inventory, connections, and verification checklist.
- `schematics/` — system block diagram, controller schematics, netlist, and design decisions.
- `tests/` — subsystem and integration test plans.
- `firmware/` — earlier planning skeletons retained for reference.

## Hardware and safety

Confirm the exact board variants, sensor interfaces, GPIO assignments, logic levels, camera configuration, and power requirements before wiring.

Do not connect mains voltage to a student-accessible breadboard. Verify power rails and low-voltage circuits before connecting the controllers or sensors.

## Validation status

Nano v12 is designated as the final Nano sketch. The ESP32-CAM source and release binaries are retained in the repository.

Compilation, flashing, and end-to-end hardware operation should be verified on the target hardware before deployment. The presence of a firmware binary does not, by itself, establish that the complete system has passed testing.

Consult the files under `docs/`, `hardware/`, `schematics/`, and `tests/` before building or modifying the system.
