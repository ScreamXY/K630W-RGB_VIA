# Redragon K630W-RGB → VIA

Custom QMK/VIA firmware for the **61-key K630W-RGB with an Evision VS11K28A controller**, plus macOS backup and flashing tools. It provides real, on-keyboard VIA remapping through [usevia.app](https://usevia.app), five editable layers, saved Windows/Mac modes, and Mac umlauts with either U.S. or EurKEY v1.2 input.

**Experimental; verified on one physical keyboard.** K630 keyboards ship with different controllers. This port is **incompatible with Sonix K630 firmware**. The model name or USB ID alone is not enough to establish compatibility. Confirm the chip marking and validate your own stock backup before flashing. Read [hardware compatibility](docs/hardware.md) and the [installation guide](docs/install.md) first.

This repository distributes source, a pinned upstream patch, and the VIA definition. It does not distribute stock firmware, personal backups, or prebuilt firmware binaries. Loading the JSON alone cannot add VIA to the stock firmware.

## Features and shortcuts

| Shortcut                | Windows mode           | Mac mode                                 |
| ----------------------- | ---------------------- | ---------------------------------------- |
| **Fn+O**                | Switch to Mac          | Switch to Windows                        |
| **Fn+I**                | Unchanged/pass-through | Toggle saved U.S. / EurKEY input profile |
| Caps Lock               | Caps Lock              | **Hyper**: Ctrl+Shift+Option+Command     |
| Fn+1…0, −, =            | F1–F12                 | Media/system controls                    |
| Fn, then Menu, then 1…= | —                      | **F1–F12**                               |
| Fn+W/A/S/D              | Up/Left/Down/Right     | Up/Left/Down/Right                       |
| Fn+H/J/K/L              | Left/Down/Up/Right     | Left/Down/Up/Right                       |
| Fn+Esc / Backspace      | Grave / Delete         | Grave / Delete                           |
| Option+A/O/U            | Normal host behavior   | **ä ö ü**; add Shift for **Ä Ö Ü**       |

“Menu” is the physical key immediately right of Fn. In Mac mode, physical **left Win is Option**, the Alt keys beside Space are Command, and Menu is Option when Fn is released. Windows retains conventional Ctrl/Win/Alt and Caps Lock behavior.

Fn+O deliberately avoids Fn+A and Fn+S so the arrow keys remain available. OS mode and input profile survive USB reconnects. Release the keys and **wait at least three seconds** before unplugging after a change. Fresh firmware starts in **Windows mode / U.S. profile**.

In Mac mode, Fn+−/= controls volume. The complete media row, lighting shortcuts, and editable layer assignments are in [the keymap guide](docs/keymap.md).

## Use VIA after installation

1. Open [usevia.app](https://usevia.app) in Chrome.
2. Enable **Settings → Show Design tab**.
3. In **Design**, load [`via/k630-vs11k28a.json`](via/k630-vs11k28a.json) as a **V3** definition; leave the V2 option disabled.
4. In **Configure**, authorize **K630W-RGB VS11K28A (experimental)**.
5. Select a layer and key, then assign its action. Changes apply immediately and save on the keyboard after a short delay.

VIA's layer buttons choose which layer to **edit**. Use Fn+O to switch the active Windows/Mac mode. The definition is loaded locally; this keyboard is not automatically recognized by VIA's public definition repository. Reauthorize after reconnecting if necessary.

Use **Save + Load → Save** to keep your own layout export. A layout export differs from the Design definition. Export before flashing: installing firmware resets VIA settings. Do not load older three- or four-layer exports into this five-layer firmware.

## Match the Mac input layout

| Selected macOS input source | Keyboard profile | What Option+A/O/U sends       |
| --------------------------- | ---------------- | ----------------------------- |
| **U.S.**                    | U.S.             | Option+U dead key, then A/O/U |
| **EurKEY v1.2**             | EurKEY           | Original Option+A/O/U chords  |

In Mac mode, **Fn+I** toggles the keyboard profile. Select the corresponding input source on the Mac yourself: the firmware cannot detect or switch the host layout. This saved selection follows the keyboard to another computer.

Both profiles produce ä ö ü, or Ä Ö Ü with Shift, with the matching host layout. Hyper and Ctrl/Command shortcuts pass through unchanged. U.S. mode replaces only these three Option shortcuts; it is not a complete EurKEY implementation. See [profile behavior and troubleshooting](docs/keymap.md#us-and-eurkey-profiles).

## Build and install

On macOS, install Python 3, Git and Xcode Command Line Tools, and start Docker Desktop's engine. Then:

```sh
git clone https://github.com/ScreamXY/K630W-RGB_VIA.git
cd K630W-RGB_VIA
python3 tools/build_firmware.py
```

The build fetches the pinned [mdmower/qmk-8051 fork](https://github.com/mdmower/qmk-8051/tree/c0d819da636af990611e874d2342f3b56e480455), applies the recovery-pin patch, compiles in Docker, packages macOS helpers, and runs offline checks. It **does not access or flash the keyboard**. The first run downloads build dependencies and may take several minutes.

Follow [installation and recovery](docs/install.md) to identify your device, make and validate a double-read stock backup, and flash deliberately. Keep another keyboard available. Entering the OEM bootloader erases its boot marker; a successful flash is required before the keyboard can type again.

## Scope and validation

The v0.4.0 implementation has been physically checked for typing, RGB, Fn arrows, VIA remapping and persistence, Windows/Mac switching, Hyper, media/F-key access, and both umlaut profiles after reconnecting. [The validation record](docs/validation.md) separates these checks from the new public build and host-tool checks.

Limitations of this 8051 port include no VIA lighting panel, mouse keys, tap-hold or one-shot actions. Lighting is controlled through Fn shortcuts or VIA custom keycodes. Macro storage is available, but playback has not been physically verified. A full physical key sweep, suspend/wake, current draw, and stock restoration remain unverified. Do not assume compatibility with another K630 revision.

## Repository guide

| Path                                                 | Contents                                                    |
| ---------------------------------------------------- | ----------------------------------------------------------- |
| [`port/`](port/keyboards/redragon/k630_vs11k28a/)    | Board matrix, five-layer keymap, OS/profile handlers        |
| [`patches/k630-8051.patch`](patches/k630-8051.patch) | Esc row-1 recovery support and macOS reset handling         |
| [`via/k630-vs11k28a.json`](via/k630-vs11k28a.json)   | VIA V3 definition and custom key names                      |
| [`tools/`](tools/)                                   | Build, local backup validation, macOS HID helpers and tests |
| [`docs/install.md`](docs/install.md)                 | Backup, install, update, recovery and troubleshooting       |
| [`docs/keymap.md`](docs/keymap.md)                   | Layers, shortcuts, input profiles and VIA customization     |
| [`docs/hardware.md`](docs/hardware.md)               | Supported hardware, matrix and flash layout                 |
| [`docs/development.md`](docs/development.md)         | Build internals and validation commands                     |
| [`docs/privacy.md`](docs/privacy.md)                 | Private local files and publication boundaries              |

Contributions and hardware reports are welcome; see [CONTRIBUTING.md](CONTRIBUTING.md). Source is provided under **GPL-2.0-or-later**; see [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md) for upstream attribution. This is an independent community project, unaffiliated with Redragon.
