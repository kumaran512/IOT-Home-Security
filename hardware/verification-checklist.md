# Hardware Verification Checklist

Use this checklist to identify the actual parts before preparing a pin-level plan. “Confirmed” below means only that the project description identifies the component and intended role; it does not mean the exact model or electrical behavior has been checked. No model numbers, voltage ratings, interfaces, or current ratings have been supplied yet.

For each item, record the marking on the part, a clear photo, the manufacturer/model, a datasheet or board pinout source, and any measurements. Do not infer ratings from appearance or from a similar module listing. Keep unverified fields blank or marked TBD; do not wire the part to a controller until its supply and signal limits are known.

## Component identification and verification record

Fill the blank fields from the physical item and its own documentation. Use one record per distinct module/part number (for example, each regulator or driver); attach extra pages if needed. Status can be `Unidentified`, `Identified`, `Source checked`, or `Measured`—include who/when in Notes. “TBD” means not yet verified.

| Component (project inventory) | Manufacturer / model | Board or part markings | Voltage rating(s) | Interface and output type | Current requirements | Datasheet / product reference | Verification status | Notes / uncertainties |
|---|---|---|---|---|---|---|---|---|
| Arduino Nano |  |  |  |  |  |  |  |  |
| ESP32-CAM board |  |  |  |  |  |  |  |  |
| Camera module / sensor |  |  |  |  |  |  |  |  |
| 0.96-inch OLED |  |  |  |  |  |  |  |  |
| Magnetic door sensor |  |  |  |  |  |  |  |  |
| IR disturbance sensor |  |  |  |  |  |  |  |  |
| Flame sensor |  |  |  |  |  |  |  |  |
| Room-indicator LEDs (3; add a row per distinct type) |  |  |  |  |  |  |  |  |
| Vibration motor |  |  |  |  |  |  |  |  |
| Servo (planned / conditional) |  |  |  |  |  |  |  |  |
| Main low-voltage supply / adapter |  |  |  |  |  |  |  |  |
| Battery / cells |  |  |  |  |  |  |  |  |
| Charger and protection / BMS |  |  |  |  |  |  |  |  |
| Power-path / switchover parts |  |  |  |  |  |  |  |  |
| Regulators (add a row per distinct part) |  |  |  |  |  |  |  |  |
| Motor / servo drivers (add a row per distinct part) |  |  |  |  |  |  |  |  |
| Signal-level interface / conditioning parts |  |  |  |  |  |  |  |  |

### Photo observations

Use one entry per photo or photo set. Select the category, note the filename and date, and record visible evidence only; a photo does not confirm electrical ratings. Use “Other / details” for multiple items in one photo or categories not listed.

| Category | Photo filename / date | Visible markings, labels, connectors, and polarity clues | Unclear or obscured details / follow-up needed |
|---|---|---|---|
| Arduino Nano |  |  |  |
| ESP32-CAM board and camera module |  |  |  |
| OLED display |  |  |  |
| Sensors (door, IR, flame) |  |  |  |
| Vibration motor and driver |  |  |  |
| Servo |  |  |  |
| Power components (supply, battery, charger, power path, regulators) |  |  |  |
| Other / details |  |  |  |

## Details required before GPIO allocation

GPIO allocation remains provisional until the relevant row below has a verified source and the proposed signal levels are compatible with the chosen board. This table describes prerequisites, not pin assignments.

| Signal or subsystem | Information required before allocating pins | Allocation status |
|---|---|---|
| Nano board resources | Exact Nano variant, MCU, pinout, logic voltage, input limits, serial/programming pins, timer/PWM needs and any board-specific restrictions | Blocked pending verification |
| Door sensor | Contact/output type, NO/NC behavior, pull-up/down arrangement, cable/noise considerations and input voltage | Blocked pending verification |
| IR sensor | Exact output type, analog/digital need, active polarity, voltage and current/drive limits | Blocked pending verification |
| Flame sensor | Exact output/interface, analog/digital need, active polarity, voltage, sensitivity and safe test method | Blocked pending verification |
| OLED | Controller, resolution, interface, pin count, address if applicable, supply and logic levels | Blocked pending verification |
| Three LEDs | Desired indication mapping, forward voltage/current, resistor selection and total I/O current | Blocked pending verification |
| Vibration motor control | Motor rated/stall current and voltage, selected driver input requirements, GPIO drive needs and suppression design | Blocked pending verification |
| Main-power status input | Verified low-voltage detector output level, isolation/conditioning, polarity and fault behavior | Blocked pending verification |
| UART between Nano and ESP32-CAM | Both board variants, selected serial resources, TX/RX levels, baud-rate capability, programming/debug conflicts and level-shifter design | Blocked pending verification |
| ESP32-CAM camera and servo | Exact board/camera pin map; pins occupied by camera, flash/PSRAM, boot straps, UART/programming; servo signal requirements | Blocked; available GPIO not established |
| Power/ground reference | Supply topology, rail voltages, driver arrangement, return currents and required common signal reference | Blocked pending power design |

Once each prerequisite is verified, update this record and `docs/pin-allocation.md` together. Record the source, date, reviewer, and any measurement for each decision. A part being present on a module does not establish that its signal is safe for a controller input.

## Safe low-voltage power-failure test

The student test must not create, switch, probe, or expose mains wiring. Use an adult-approved, certified enclosed adapter as the normal source. Test the detector at its designed **low-voltage output** using one of these methods, chosen to match the verified detector design:

1. With the circuit unpowered, disconnect the adapter's low-voltage output using its intended connector or switch; keep the adapter enclosure closed and mains plug inaccessible to the student during wiring changes. Reconnect the low-voltage output to restore normal power.
2. If the detector accepts a separate low-voltage sense input, use a current-limited, isolated low-voltage bench source or a documented test switch at that input. Stay within the detector's verified input range and wiring instructions.
3. If the design uses power-path switchover, simulate loss by removing only the normal low-voltage adapter output through its connector/switch and allow the already approved battery path to take over. Do not short rails, bypass the charger/BMS, or connect supplies in parallel outside the designed power path.

Before the test, check the detector and supply datasheets, polarity, voltage range, current limit, battery protection, and probe points. Use a multimeter only on documented low-voltage test points. Record the normal and failed-state detector output, rail voltages, Nano behavior, and restoration behavior. Stop if the topology or test points are uncertain, a battery or component heats, or voltage is outside its documented range. Do not open an adapter or connect a mains-side test switch, divider, or sensor to a breadboard.
