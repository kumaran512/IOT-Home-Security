# Power-Management Plan

This plan is intentionally topology-neutral until the exact supply, battery, board variants, and load currents are known. Do not connect a battery or charger based only on this document.

## Rails and loads to define

- Confirm the input supply and the allowed input path for each controller board. ESP32-CAM camera operation can have significant current peaks; use the board maker's supply guidance and verify stability under Wi-Fi/camera load.
- Make a load budget from measured or datasheet current for Nano, ESP32-CAM, OLED, sensors, LEDs, vibration motor, and servo. Include startup and stall/surge current, not just nominal current.
- Use regulated rails with adequate transient response and current capacity. Do not power a motor or servo from a GPIO pin or assume the Nano/ESP32 board regulator can supply them.
- Prefer a separately sized motor/servo supply path. Use a suitable transistor/MOSFET or motor driver for the vibration motor and an appropriate servo supply. Place required suppression components according to driver/load documentation. Join grounds appropriately for control signals while managing return currents to limit resets/noise.

## Main-power failure and backup

- Detect loss of the low-voltage system input with a properly conditioned, isolated signal appropriate to the chosen supply. Never connect mains directly to a microcontroller pin, breadboard, or exposed student wiring.
- Define which loads remain active on backup. Prioritize Nano sensing and local alerting; camera, Wi-Fi, servo, and display may need to be shed if battery capacity is limited.
- Select battery chemistry and cell count, capacity, protection/BMS, charger, and power-path/OR-ing arrangement as a matched system. Use a charger explicitly intended for that battery chemistry and configuration; do not parallel supplies or batteries without a designed power path.
- Size runtime using load budget, usable battery energy, regulator efficiency, and conservative derating. Verify voltage remains in each board's input range during transitions and alerts.
- If a mains-powered supply is used, use a certified enclosed adapter. Keep all mains-voltage wiring out of student-accessible breadboards and low-voltage prototype areas.

## Verification before use

Record rail voltages and current limits, observe voltage dips during camera startup/transmission and motor/servo movement, test main-power switchover, and verify no reset or unsafe battery/charger heating. Exact limits and acceptance values depend on selected parts and remain TBD.
