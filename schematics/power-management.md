# Power Management (Preliminary)

This document is a proposed architecture only. It is **not a power schematic ready to build**. The reported 5 V / 5 A supply, 5 V / 5,000 mAh power bank, and 5 V active-low relay module have not been electrically verified. Keep mains wiring and mains conversion outside the student-accessible circuit; only enclosed, certified AC/DC equipment may be used upstream.

## Proposed low-voltage architecture

```text
MAIN_5V (reported 5 V, 5 A) -----> [power mux / ideal-diode selector] -----> 5V_SYS_LOGIC
                                                   ^                          |
USB_BANK_5V (reported 5 V) ------------------------+                          +--> Nano input path (verify)
                                                                              +--> ESP32-CAM input path (verify)
                                                                              +--> OLED and sensors (verified rails only)
                                                                              +--> protected ACTUATOR branch (budget first)

MAIN_5V present-state detector ---> relay control / contacts ---> MAIN_OK_SENSE ---> Nano input
```

Use a purpose-designed dual-input power mux or ideal-diode/power-path assembly with reverse-current blocking; do not directly parallel `MAIN_5V` and `USB_BANK_5V`, and do not use the relay contacts as the source-combining element. Specify a part only after the voltage range, worst-case total/peak current, reverse leakage, transition behavior, thermal dissipation, and source behavior are known. The power bank's pass-through and auto-shutdown behavior may constrain or prevent automatic backup.

Separate `5V_SYS_LOGIC` and `ACTUATOR_5V` as named branches with appropriate protection/decoupling once loads are known. If the actuator branch is sourced from the selected system rail, the selector and source must support simultaneous camera, servo, and motor transients; otherwise design a suitable separately regulated actuator source. No independent rail rating is assumed here.

## Relay as an indication component

The single-channel relay's reported **active-low** property describes the control input polarity only: the input is asserted low to request the relay's energized state, subject to its actual thresholds and circuit. It does not itself detect loss of the main supply. A separate main-present sensing condition must decide when its input is asserted, or a documented coil-power/contact behavior must be deliberately used as the detector.

One candidate detection sequence, only if the actual module supports it, is: power the relay module/coil from `MAIN_5V`; assert its active-low control while main power is within a verified “present” range; let the relay release when that supply disappears; and sense the resulting contact state with a backup-powered Nano input. `COM`, `NO`, and `NC` must be identified from the actual module and energized/de-energized measurements; which state maps to `MAIN_OK` is TBD. The contact signal must be biased and clamped to a known Nano-safe logic range. Verify the module's input circuit, coil supply arrangement, and whether the contact is actually isolated before using this concept. If the relay cannot be powered/control-conditioned this way, use a separately selected low-voltage monitor; do not infer a detector from the active-low label.

The relay module is **not** the backup power selector. Its presence does not establish safe or seamless changeover. Keep detection and source selection as separate circuit functions and validate them independently.

## Loads and sizing inputs

| Load | Preliminary rail/interface | Required information before sizing |
|---|---|---|
| Arduino Nano | Board-approved input from `5V_SYS_LOGIC` | Exact board revision/input path, operating current, USB backfeed behavior, regulator limits |
| ESP32-CAM + camera + Wi-Fi | Board-approved `5V_SYS_LOGIC` input | Exact board/SoC/camera, startup/Wi-Fi peak, rail tolerance, connector path |
| OLED and sensors | Verified compatible logic/sensor supply | Each module's operating range, output high/low, current, pull-up rails |
| Three LEDs | Logic rail through individual resistors/driver | LED forward voltage and target current; GPIO/driver limits |
| Two-wire vibration motor | `ACTUATOR_5V` through a driver | Motor type, rated/startup/stall current, voltage, suppression requirement |
| SG90 servo | `ACTUATOR_5V` (conditional) | Exact servo variant, voltage range, moving/stall current, signal threshold, simultaneous load |

Derive the source, selector, connector, wiring, protection, and regulator ratings from the combined continuous load plus startup/stall/transient load with margin. The stated 5 A main source and 5,000 mAh power bank do not establish usable capacity or current capability.

## Verification gate

Before a physical build: identify the main source construction/protection; verify power-bank manufacturer/model, energy rating, usable 5 V capacity, maximum output current and pass-through behavior; inspect relay model, input thresholds, isolation, contacts and terminal labels; determine a real main-failure sensing method; select a power mux; calculate load budget; and check each controller's approved input path. Keep Nano monitoring and local alerting operational without ESP32-CAM/Wi-Fi. Test low-voltage rails and changeover using current-limited equipment only after the circuit is reviewed.
