# Hardware compatibility

The working port was developed against one 61-key ANSI **Redragon K630W-RGB** with an **Evision VS11K28A** MCU. The chip marking was inspected on the PCB; the stock firmware was read twice with identical results. The MCU uses the 8051/CH555 backend in the pinned QMK fork.

There are incompatible K630 controller variants. In particular, Sonix K630 revision-1 firmware is not suitable for this board. A model label, purchase date, case color or stock USB ID does not prove compatibility. With power disconnected, confirm the MCU marking and any PCB revision before using this port. This repository does not claim a universal disassembly screw layout.

## USB interfaces

| Mode               | VID:PID     | Interface used by these tools                     |
| ------------------ | ----------- | ------------------------------------------------- |
| Stock candidate    | `320f:5000` | Vendor collection `ff1c:0092`, report 4, 64 bytes |
| Evision bootloader | `0c45:7687` | 64-byte feature reports                           |
| This QMK port      | `5244:0630` | VIA Raw HID `ff60:0061`, 32 bytes                 |

The stock and bootloader IDs are shared with other keyboards. Tools require a selected USB location, a recognized backup, and the expected interface. This is a compatibility guard, not cryptographic device authentication. The public build leaves the serial unspecified so upstream derives it from the chip ID at runtime.

## Matrix and recovery input

- Six rows on `P4.0`–`P4.5`.
- Sixteen columns on `P0.0`–`P0.7` and `P3.0`–`P3.7`.
- 61 populated positions, listed in [`keyboard.json`](../port/keyboards/redragon/k630_vs11k28a/keyboard.json).
- The ROM default table at `0x6210 + (column × 8 + row) × 3` matches every physical key position. Saved user profiles are deliberately not used to identify the default matrix.
- **Esc is row 1, column 0: `P4.1/P0.0`.** The upstream default recovery stub reads row 0. The supplied patch adds `EVISION_ESC_ROW=1`; the validation suite checks that only the two required bit operands differ.

Never flash the unmodified upstream row-0 recovery stub onto this board. The recovery input runs before QMK or VIA and does not depend on the current Esc mapping in VIA.

## Flash layout

| Address range   | Purpose                                                             |
| --------------- | ------------------------------------------------------------------- |
| `0x0000–0x27BF` | OEM bootloader code; never programmed by the app flasher            |
| `0x27C0–0x27FF` | Boot marker page; valid marker begins `AA 55 33 CC`                 |
| `0x2800`        | App entry / recovery jump                                           |
| `0x3000`        | Interrupt-vector table                                              |
| `0x3027`        | Linked QMK code start                                               |
| `0xDF40–0xEFBF` | VIA EEPROM emulation: two slots with headers for a 2,048-byte store |
| `0xF000–0xF3FF` | Lighting data region, outside the app image                         |

The app flasher is bounded to `0x2800–0xEFBF`. Its erase resets the VIA settings store. The packaged QMK code must finish before `0xDF40`. A full 64 KiB dump is rejected as a flash image.

`prepare_backup.py` recognizes the tested bootloader code fingerprint, excluding the mutable marker page:

```text
SHA-256 of bytes 0x0000–0x27BF:
41f96f863d081044a847ae99d94f1b02cdf1b85b4f3f6062a92e4737ee76af18
```

This is a firmware-family signature, not a serial number. The validator additionally checks the marker, all 61 ROM keys and app vectors. A different fingerprint requires investigation and an independently validated port, not a new hash pasted into the whitelist.

## Remaining hardware limits

A single unit passing checks does not validate other PCB revisions. The [validation record](validation.md) lists what was actually tested. Power draw, indicator behavior, full physical matrix coverage, suspend/wake, and restoration to stock need further hardware testing. The descriptor declares 200 mA and the initial lighting intensity is reduced, but neither proves measured electrical consumption.
