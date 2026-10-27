# Power-Management Plan

This plan is intentionally topology-neutral until the exact supply, battery, board variants, and load currents are known. Do not connect a battery or charger based only on this document.

## Preliminary architecture (not verified)

The user describes a DIY 5 V, 5 A main supply intended to power the setup, a DIY 5 V battery, a 5 V, 5 A charger, and a single-channel 5 V relay module. These are user-reported descriptions, not verified ratings or evidence of a compatible circuit. Treat them as candidate components only. The intended concept is to power the system from the main low-voltage source and provide backup for Nano sensing and local alerts if that source fails. Keep Nano local alerts operational independently of the ESP32-CAM and Wi-Fi. Which other loads remain powered is not yet determined.

Battery charging, power-path selection, and automatic changeover are not verified. A relay module must not be assumed to provide safe or seamless power changeover: inspect its circuit, input requirements, contact arrangement (including COM/NO/NC), isolation, switching behavior, and ratings against the actual sources and load before designing or testing a power path. The relay description alone does not establish battery charging control, source isolation, uninterrupted transfer, or protection. Do not finalize wiring or connect these sources until the battery chemistry, charger compatibility, protection, source behavior, and complete power path have been reviewed.

## Rails and loads to define

- Confirm the input supply and the allowed input path for each controller board. ESP32-CAM camera operation can have significant current peaks; use the board maker's supply guidance and verify stability under Wi-Fi/camera load.
- Make a load budget from measured or datasheet current for Nano, ESP32-CAM, OLED, sensors, LEDs, vibration motor, and servo. Include startup and stall/surge current, not just nominal current.
- Use regulated rails with adequate transient response and current capacity. Do not power a motor or servo from a GPIO pin or assume the Nano/ESP32 board regulator can supply them.
- Prefer a separately sized motor/servo supply path. Use a suitable transistor/MOSFET or motor driver for the vibration motor and an appropriate servo supply. Place required suppression components according to driver/load documentation. Join grounds appropriately for control signals while managing return currents to limit resets/noise.

## Main-power failure and backup

- Detect loss of the low-voltage system input with a properly conditioned, isolated signal appropriate to the chosen supply. Never connect mains directly to a microcontroller pin, breadboard, or exposed student wiring.
- Define which loads remain active on backup. Prioritize Nano sensing and local alerting; camera, Wi-Fi, servo, and display may need to be shed if battery capacity is limited.
- Select battery chemistry and cell count, capacity, protection/BMS, charger, and power-path/OR-ing arrangement as a matched system. Use a charger explicitly intended for that battery chemistry and configuration; do not parallel supplies or batteries without a designed power path.
- Verify the DIY supply's construction, regulation, current capability, and protection; verify the battery's chemistry, cell configuration, capacity, discharge limits, and protection; and verify the charger's topology, charge profile, and compatibility. Stated “5 V, 5 A” descriptions do not establish these properties.
- Size runtime using load budget, usable battery energy, regulator efficiency, and conservative derating. Verify voltage remains in each board's input range during transitions and alerts.
- If a mains-powered supply is used, use a certified enclosed adapter. Keep all mains-voltage wiring out of student-accessible breadboards and low-voltage prototype areas.

## Verification before use

Record rail voltages and current limits, observe voltage dips during camera startup/transmission and motor/servo movement, test main-power switchover, and verify no reset or unsafe battery/charger heating. Exact limits and acceptance values depend on selected parts and remain TBD.
