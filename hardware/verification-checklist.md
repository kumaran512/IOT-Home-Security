# Hardware Verification Checklist

Use this checklist to identify the actual parts before preparing a pin-level plan. User-provided markings and descriptions below are observations/reports, not independent verification of exact models or electrical behavior. Electrical ratings and board-specific details remain unverified unless documented here with a source or measurement.

For each item, record the marking on the part, a clear photo, the manufacturer/model, a datasheet or board pinout source, and any measurements. Do not infer ratings from appearance or from a similar module listing. Keep unverified fields blank or marked TBD; do not wire the part to a controller until its supply and signal limits are known.

## Component identification and verification record

Fill the blank fields from the physical item and its own documentation. Use one record per distinct module/part number (for example, each regulator or driver); attach extra pages if needed. Status can be `Unidentified`, `Identified`, `Source checked`, or `Measured`—include who/when in Notes. “TBD” means not yet verified.

| Component (project inventory) | Manufacturer / model | Board or part markings | Voltage rating(s) | Interface and output type | Current requirements | Datasheet / product reference | Verification status | Notes / uncertainties |
|---|---|---|---|---|---|---|---|---|
| Arduino Nano |  | ATmega328PU marking; Mini-USB connector (user-observed) | TBD | TBD; exact pin order TBD | TBD |  | User-observed markings; exact board revision, clock, bootloader, ratings, and pin order pending verification |  |
| ESP32-CAM board |  | Mini-USB connector (user-observed) | TBD | TBD; GPIO mapping and exact pin order TBD | TBD |  | User-observed connector; exact board revision and ratings pending verification |  |
| Camera module / sensor |  | Ribbon marking: TY-OV2 640-V2.0 (user-observed) | TBD | TBD | TBD |  | Ribbon marking user-observed; camera sensor identification and ratings pending verification |  |
| 0.96-inch OLED |  | GM009605 (user-observed) | TBD | I²C (user-reported); exact pin order TBD | TBD |  | Size 0.96 inch, 128 × 64, monochrome, SSD1306 controller, and I²C are user-reported; electrical details pending verification |  |
| Magnetic door sensor |  | Magnetic reed switch (user-reported) | TBD | Switch/contact; NO/NC behavior TBD | TBD |  | Reed type user-reported; NO/NC behavior and electrical limits pending verification |  |
| IR disturbance sensor |  |  | TBD | Binary output (user-reported); output voltage/polarity TBD | TBD |  | Output type user-reported; electrical limits and exact module pending verification |  |
| Flame sensor |  | VCC, GND, DO terminals (user-reported) | TBD | DO reported as binary output; output voltage/polarity TBD | TBD |  | Terminals and binary-output description user-reported; electrical limits and module details pending verification |  |
| Room-indicator LEDs (3; add a row per distinct type) |  |  |  |  |  |  |  |  |
| Vibration motor |  | Two-wire motor (user-reported) | TBD | Two-wire motor; driver/interface TBD | TBD; motor ratings pending verification |  | Construction user-reported; operating/stall ratings pending verification |  |
| Servo (planned / conditional) |  | Blue SG90 (user-reported) | TBD | TBD | Current requirements pending verification |  | Color/model user-reported; exact variant, supply, and servo current pending verification |  |
| Main low-voltage supply / adapter |  | DIY supply, described as 5 V, 5 A (user-reported) | 5 V, 5 A stated; output and ratings unverified | Intended to power setup; topology, connector, polarity TBD | 5 A stated capacity unverified; available continuous/transient current TBD |  | User-described rating only; design, regulation, protection, and safe current capacity pending verification |  |
| Backup source / power bank |  | USB power bank (user-reported); manufacturer/model TBD | 5 V output, 5,000 mAh rating (user-reported; energy rating unverified) | USB output; pass-through behavior TBD | Maximum output current and actual usable 5 V capacity unverified |  | User-reported description only; manufacturer/model, energy rating, usable capacity, maximum current, and pass-through behavior pending verification |  |
| Charger and protection / BMS |  | Charger described as 5 V, 5 A (user-reported) | 5 V, 5 A stated; charge profile/limits TBD | Charger type and battery compatibility TBD | 5 A stated rating unverified; actual charge current TBD |  | User description only; charger type, compatibility, and protection pending verification |  |
| Power-path / switchover parts |  | Single-channel, 5 V, active-low relay module (user-reported) | 5 V stated for module; coil/contact ratings TBD | Active-low control polarity (user-reported); exact input thresholds, isolation, and terminal arrangement (COM/NO/NC) TBD. Control polarity does not establish power-failure detection or automatic changeover. | Coil and contact ratings TBD |  | Exact model, control-input thresholds, isolation, contact ratings, terminal arrangement, and changeover behavior pending verification; module alone is not evidence of seamless backup |  |
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
