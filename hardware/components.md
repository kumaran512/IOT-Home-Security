# Component Inventory

This is the project-level inventory; component models and electrical ratings have not been provided or independently confirmed. Use [the hardware verification checklist](verification-checklist.md) as the detailed record before wiring or allocating GPIOs. “Arduino Nano,” “ESP32-CAM,” and the stated component roles are confirmed only as project requirements, not exact hardware variants.

| Component | Intended role | Details to record before wiring |
|---|---|---|
| Arduino Nano | Primary sensor/alert controller | Manufacturer/revision, microcontroller, logic voltage, input limits and regulator/current limits |
| ESP32-CAM and camera | Independent camera and Wi-Fi AP | Board/module and camera variants, flash/PSRAM, antenna, pin map and supply requirements |
| 0.96-inch OLED | Local status | Controller, resolution, interface, supply/logic levels, address and current |
| Magnetic door sensor | Door state | Contact/module type, NO/NC behavior, electrical output, cable and pull-up needs |
| IR disturbance sensor | Disturbance input | Exact module, output type/voltage/polarity, range and adjustment |
| Flame sensor | Flame/hazard input | Exact sensor/module, output interface/voltage, sensitivity and limitations |
| Three LEDs | Room/status indicators | Color, forward voltage/current, resistor values and indicator mapping |
| Vibration motor | Local alert | Model, rated/stall current, voltage, driver and suppression needs |
| Servo | Planned camera positioning | Model, voltage, moving/stall current, torque and travel limits |
| Main supply | Low-voltage input | Source/model, output tolerance/current, connector and polarity |
| Battery, charger and power path | Backup | Chemistry, series count, capacity, protection, compatible charger and switchover design |
| Regulators, drivers and level interface | Conditioning | Part/model, input/output range, current, logic thresholds and thermal/transient limits |

Every electrical value remains TBD until supported by a part marking, datasheet, or documented measurement. Preserve the source and measured results with the project before finalizing the connection notes.
