# UART Communication Protocol Plan (Provisional)

This is a small, human-readable protocol proposal for later implementation. It is not implemented or validated. Confirm baud rate, serial port availability, and framing against the selected Nano and ESP32-CAM boards. Both sides must continue their core functions if the link is absent.

## Proposed transport

- Asynchronous UART, 8 data bits, no parity, 1 stop bit (8N1); initial baud rate **TBD during bring-up**.
- One line per message, terminated by LF (`\n`); accept optional CR before LF.
- ASCII only. Keep lines short enough for fixed, modest buffers on both boards.
- Nano is the event source. ESP32-CAM may emit a readiness/status line; neither side blocks waiting for a reply.
- UART voltage levels must match. If Nano TX is 5 V and the ESP32-CAM RX is 3.3 V-only, use a suitable level shifter or divider designed for the actual baud rate. ESP32 TX at 3.3 V into the selected Nano RX must also be checked for input-high margin. Share signal ground where the supply topology requires it.

## Proposed message grammar

`V1,<sequence>,<event>,<value>\n`

- `V1`: protocol version.
- `sequence`: unsigned decimal counter, wraps at a documented width; gaps are allowed.
- `event`: restricted uppercase token, e.g. `DOOR`, `IR`, `FLAME`, `POWER`, `HEARTBEAT`.
- `value`: short token such as `OPEN`, `CLEAR`, `TRIGGERED`, `FAIL`, `RESTORED`, or `OK`.
- Reject malformed/oversized lines; discard until next LF after overflow. No dynamic allocation is needed.

Examples are illustrative only: `V1,12,DOOR,OPEN` and `V1,13,POWER,FAIL`.

## Reliability and implementation decisions

- Receiver should parse incrementally with a bounded buffer and timeout-free operation; incomplete input must not stall sensor polling.
- Treat duplicate or missing sequence values as diagnostics, not conditions that stop local alerting.
- Decide whether event changes only or periodic heartbeat is required; define event debounce and retry policy during implementation.
- No security, authentication, or remote command protocol is implied. Do not send commands that can disable local alarms over this link.
