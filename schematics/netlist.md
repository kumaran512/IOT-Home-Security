# Proposed Netlist (Logical / Preliminary)

Names below define intended signal roles, not permission to wire. Component ratings, connector pin order, conditioning values, and several board pins are intentionally TBD. All `GND_*` commoning is conditional on confirming the power-path and interface isolation requirements.

## Rails and returns

| Net | Proposed members | Requirements / TBD |
|---|---|---|
| `MAIN_5V` | Main source output; selector input A; main-present detector input | User-described 5 V / 5 A. Verify regulation, tolerance, current, protection, polarity, connector. |
| `USB_BANK_5V` | Power-bank USB output; selector input B | User-described 5 V, 5,000 mAh. Verify usable 5 V capacity, current, pass-through, cable/connector. |
| `5V_SYS_LOGIC` | Selector output; approved Nano/ESP32-CAM input paths; compatible OLED/sensor branches | Mux output/peak current and transient behavior TBD. No direct source paralleling. |
| `ACTUATOR_5V` | Protected branch to motor driver and SG90 | May be derived from selected source only if worst-case load budget permits; voltage/current and protection TBD. |
| `GND_LOGIC` | Nano, ESP32, sensor logic, OLED, UART interface, driver control returns | Shared signal reference where interfaces are non-isolated. |
| `GND_ACTUATOR` | Motor/servo return and driver power return | Return routing/ground connection to `GND_LOGIC` determined by driver/interface and layout. |

## Nano logical connections

| Source | Destination | Net / proposal | Requirements |
|---|---|---|---|
| Reed contact | `GND_LOGIC` and Nano D2 input | `DOOR_SW_N` | Pull-up to verified Nano logic rail; NO/NC, contact state, cable conditioning TBD. |
| IR sensor output | Nano D3 via input conditioner | `IR_DO` | Verify supply, polarity, output topology, high/low levels and powered-off behavior. |
| Flame module DO | Nano D4 via input conditioner | `FLAME_DO` | Sensor VCC/GND/DO user-reported; verify VCC range and DO levels/polarity. |
| Nano A4 / A5 | OLED SDA / SCL | `I2C_SDA` / `I2C_SCL` | Conditional classic Nano mapping; verify OLED pin order/address and pull-up voltage. |
| Nano D5 / D6 / D7 | LED branches | `ROOM_LED_1..3` | One series resistor per LED, calculated after Vf/current and output-driver limits known. |
| Nano D8 | Motor driver input | `MOTOR_DRV_EN` | Driver logic level compatible; motor supply current never from GPIO. |
| Relay/detector contact interface | Nano A0 | `MAIN_OK_SENSE` | Condition/bias from known surviving logic rail; contact arrangement and detection topology TBD. |
| Nano D1 TX | level shifter input | `NANO_TX_5V` | D1 hardware TX conditional classic Nano; programming/USB serial sharing managed. |
| level shifter output | ESP32 RX selected after audit | `ESP32_RX_3V3` | 5 V-to-3.3 V direction. No direct 5 V ESP32 input. |
| ESP32 TX selected after audit | Nano D0 RX | `ESP32_TX_3V3` | Verify Nano high threshold; add compatible translation if required. |

## Driver and camera connections

| Source | Destination | Net / proposal | Requirements |
|---|---|---|---|
| `ACTUATOR_5V` | Motor driver power input | `MOTOR_V+` | Driver selected from motor type, startup/stall current and supply. Add suppression specified by load/driver documentation. |
| Motor driver output | Two motor wires | `MOTOR_A`, `MOTOR_B` | Never connect either motor lead to MCU GPIO. Confirm if polarity reversal is needed; likely not assumed. |
| `ACTUATOR_5V` / return | SG90 power leads | `SERVO_V+`, `SERVO_GND` | Exact lead order, voltage/current, protection and common reference verified from servo docs. |
| ESP32 selected output via signal interface | SG90 signal lead | `SERVO_PWM` | GPIO intentionally TBD pending board, camera, flash/PSRAM, boot and programming map. |
| ESP32-CAM camera connector | TY-OV2 640-V2.0 ribbon | `CAMERA_IF` | Identify sensor/connector pinout/orientation; board-defined mapping preserved. |

The relay's control input (`RELAY_IN_N`) is separate from its switched contacts (`RELAY_COM`, `RELAY_NO`, `RELAY_NC`). The reported active-low control polarity applies only to `RELAY_IN_N`. Do not treat these signal names as a finalized power-changeover netlist.
