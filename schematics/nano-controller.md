# Nano Controller Connections (Preliminary)

The following pin assignments are a **provisional allocation proposal**, assuming a classic ATmega328P Nano-compatible board with the marked ATmega328PU, Arduino-style pin numbering, and the expected I²C/UART mapping. They are not approved for wiring until board revision, logic voltage, pinout, and onboard USB-serial connections are checked. The reported Mini-USB connector alone does not prove a classic Nano layout.

| Nano pin | Proposed signal | Connection / condition |
|---|---|---|
| D2 | `DOOR_SW_N` | Reed switch to verified common ground; use a pull-up to the Nano logic rail and debounce/long-cable conditioning as needed. Contact NO/NC state is unknown. |
| D3 | `IR_DO` | Binary sensor output through level-compatible input conditioning; confirm output voltage, polarity, and whether output is push-pull/open-collector. |
| D4 | `FLAME_DO` | Flame module DO through level-compatible input conditioning; power VCC only from its verified supply range. |
| D5 | `ROOM_LED_1` | GPIO to LED driver/current path with its own series resistor; polarity and current TBD. |
| D6 | `ROOM_LED_2` | As above, separate resistor. |
| D7 | `ROOM_LED_3` | As above, separate resistor. |
| D8 | `MOTOR_DRV_EN` | Driver input only; never connect the motor to this GPIO. |
| A0 | `MAIN_OK_SENSE` | Conditioned low-voltage status from the verified power-failure detector/relay contact interface. Do not connect raw supply or relay coil here. |
| A4 / A5 | `I2C_SDA` / `I2C_SCL` | GM009605 OLED, conditional on verified pin order, supply, logic levels, and pull-up rail. |
| D1 / D0 | `NANO_TX_5V` / `NANO_RX` | Hardware UART proposal. TX path to ESP32 requires 5 V-to-3.3 V level interface; RX high-level compatibility must also be verified. |
| 5V / GND | `5V_SYS` / `GND_LOGIC` | Feed only through the selected board's verified input path. Common reference required for non-isolated signal interfaces. |

The classic Nano I²C mapping A4/A5 and UART mapping D0/D1 must be confirmed against the actual clone/board pinout. Arduino's classic Nano documentation lists Mini-B USB and its I²C documentation maps Nano SDA/SCL to A4/A5 ([Nano](https://docs.arduino.cc/hardware/nano), [I²C pin mapping](https://docs.arduino.cc/learn/communication/wire)).

## Interface notes

- The reed switch is a dry contact; proposed D2 pull-up wiring gives a logic transition, but whether that represents door-open or door-closed depends on reed NO/NC behavior and magnet placement.
- Do not connect sensor outputs until their high level, low level, output topology, powered/unpowered behavior, and Nano logic voltage are known. Add a divider, buffer, transistor, or optocoupler only after selecting it from those values.
- For each LED calculate/select `R = (Vdrive - Vf - driver drop) / I_LED` from measured/verified LED forward voltage, desired current, and driver/output limits. Do not use one resistor shared across LEDs.
- Use a low-side transistor/MOSFET or rated driver for the motor. Select the switch and suppression from motor type, operating/stall current, supply, and switching behavior. If it is a brushed inductive motor, provide correctly oriented flyback suppression at the load/driver as its datasheet requires.
- The OLED may contain pull-ups to its VCC. Verify those pull-ups do not expose the Nano or other bus participant to an incompatible voltage; add a bidirectional I²C level shifter if required by actual levels.
- `D0/D1` are also the usual hardware UART and commonly share the board USB-serial programming path. Plan a disconnect/jumper or defined isolation procedure during upload and debugging; verify the exact board schematic first.

No resistor, transistor, protection-device, or supply rating is selected here. Required parameters are listed in [netlist](netlist.md) and [design decisions](design-decisions.md).
