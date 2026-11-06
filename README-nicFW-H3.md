# TIDRADIO TD-H3 (nicFW 2.5x) support for flrig

This tree is flrig 2.0.12 with an added transceiver, **TD-H3 nicFW**, for the
TIDRADIO TD-H3 running nicsure's nicFW 2.5x firmware (written against the
published 2.5 source and checked against the V2.52.221 binary).

New files:

- `src/include/other/nicFW_H3.h`
- `src/rigs/other/nicFW_H3.cxx`

Registered in `src/Makefile.am`, `src/include/rigs.h` and `src/rigs/rigs.cxx`.

## Build

```bash
autoreconf -fi
```

```bash
./configure
```

```bash
make
```

## Setup

Config > Setup > Transceiver: select **TD-H3 nicFW** and the programming
cable's serial port. The defaults are 38400 baud, 1 stop bit, and PTT via CAT.

## What works

| Function | How it is done |
|---|---|
| Read VFO A/B frequency, modulation, bandwidth | EEPROM reads (`0x30`). The radio saves the active VFO to EEPROM on every change |
| Channel mode | Shows the selected channel's frequency (read-only) |
| Set frequency | Simulated keypad entry (`0x4C 0x4D key`): MHz digits, `*`, 5 decimals. 10 Hz resolution, takes about 3–4 s |
| A/B select | Simulated `#` key |
| PTT | Holds the remote PTT A or PTT B key for the active VFO |

## Limitations (firmware, not flrig)

- **No S-meter.** `uart.c` has a `0xAA` VFO-status/RSSI command, but
  `uartIdle()` sends every byte ≥ `0x80` to the remote-key handler before it
  reaches the command switch. That code can never run, in the source or in
  the V2.52.221 binary.
- **Modulation and bandwidth can't be changed from flrig.** They are only
  reachable through the radio's menu.
- **Frequency can only be set in VFO (frequency) mode.** If a VFO is in
  channel mode, flrig logs a warning and leaves it alone.
- Each frequency change is about ten simulated key presses, so the radio
  beeps if key tones are on. Don't sweep the frequency with the mouse wheel.
- Don't use the radio's keypad while flrig is setting a frequency.
- Don't run the nicFW remote app (remote mode `0x4A`) at the same time as
  flrig.
