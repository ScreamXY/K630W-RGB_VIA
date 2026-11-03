# Development and validation

## Source and build

This is a board overlay and a small patch over [mdmower/qmk-8051](https://github.com/mdmower/qmk-8051), not a fork containing the whole QMK tree.

| Dependency | Pinned revision                                     |
| ---------- | --------------------------------------------------- |
| qmk-8051   | `c0d819da636af990611e874d2342f3b56e480455`          |
| QMK LUFA   | `549b97320d515bfca2f95c145a67bd13be968faa`          |
| SDCC       | `4.6.0`, selected by the pinned upstream Dockerfile |

Run `python3 tools/build_firmware.py`. The script creates `build/qmk-8051`, fetches the source, applies `patches/k630-8051.patch`, initializes LUFA, copies the overlay, and invokes upstream's `util/ch555_build.py redragon/k630_vs11k28a via` with `DOCKER_DEFAULT_PLATFORM=linux/amd64`.

It packages the firmware and patched image/flashing validators into `build/`, writes SHA-256 hashes to `build/manifest.json`, and runs the offline tests. On macOS it also compiles the native helpers and runs the backup reader's exhaustive framing self-test. No part of this build accesses USB.

The published builder does not bake in a keyboard serial. The firmware uses upstream's runtime chip-ID serial. The local manifest always starts with `hardware_tested: false`: compiling and passing tests cannot certify a newly built image on hardware. Build timestamps and toolchain dependencies can change hashes; bit-for-bit reproducible builds are not claimed. The upstream Dockerfile and source are pinned, but the resulting Docker image and every downloaded package are not content-pinned by this project.

The build wrapper skips macOS helpers on other hosts. A Linux source build requires Python 3, Git, clang and Docker; it has not been exercised here. The provided backup/install workflow is macOS-only. Do not treat the Mac host tools as Linux or Windows flashing instructions.

To rebuild the native helpers without another firmware compile, after a successful build:

```sh
python3 tools/build_firmware.py --package-only
```

This updates the local manifest and does not access USB. For a completely fresh build, preserve your backups and move the disposable `build/` directory aside before running the builder. Never store your only recovery copy inside `build/`.

## Tests

After building:

```sh
python3 -m unittest discover -s tools -p 'test_*.py' -v
build/evision_backup_macos --self-test
```

The suite checks the linked image, matrix/VIA coordinates, OS switching, both umlaut profiles, modifier restoration, release behavior, layer access, Esc stub, image bounds, flash packet payloads, checksum failures and reset/re-enumeration behavior. Public tests use simulated USB transactions and never send keyboard commands.

The stock-ROM test is optional because proprietary stock firmware is not distributed. Run it against your own verified full backup:

```sh
K630_STOCK_BACKUP=backups/stock-full64k.bin python3 -m unittest discover -s tools -p 'test_*.py' -v
```

On a Mac with U.S. and EurKEY v1.2 installed, the additional translation check exercises actual macOS layout tables without selecting a source or injecting input:

```sh
clang -Wall -Wextra -Werror -framework Carbon tools/test_macos_layouts.c -o build/test_macos_layouts
build/test_macos_layouts
```

Its EurKEY input-source ID is `org.unknown.keylayout.EurKEYv12`. A different EurKEY distribution may use a different ID and needs that test adjusted. This optional check requires both layouts to be installed.

## Code map

| File                            | Responsibility                                                      |
| ------------------------------- | ------------------------------------------------------------------- |
| `keyboard.json`, `led_fx_pos.h` | Physical matrix and LED positions                                   |
| `keymaps/default/keymap.c`      | Five default layers, reused by the VIA keymap                       |
| `os_mode.c`                     | Persistent Windows/Mac selection; Win-lock scope; VIA diagnostics   |
| `umlaut.c`                      | Saved Mac input profile; U.S. composition and EurKEY pass-through   |
| `k630_vs11k28a.c`               | Board initialization, key-handler chain, lighting and suspend hooks |
| `keymaps/via/config.h`          | Five-layer storage version and firmware version                     |
| `patches/k630-8051.patch`       | Esc row selection, K630 ID recognition, reset-error verification    |
| `tools/prepare_backup.py`       | Offline compatibility and local recovery-record validation          |
| `tools/flash_macos.py`          | Manifest/backup checks and bound-port upstream flash transaction    |
| `tools/k630_hid_macos.c`        | Narrow native HID transport                                         |
| `tools/evision_backup_macos.c`  | Stock read protocol only; double-read equality before saving        |
| `tools/via_probe_macos.c`       | Read-only mode and keymap diagnostics                               |

## Persistent settings and VIA protocol

The default layer stores Windows (`0`) or Mac (`2`) using QMK's persistent default-layer API. Bit 0 of `eeconfig_read_kb()` stores the input profile: `0` for U.S., `1` for EurKEY. Profile updates preserve other bits. Changing OS does not change the profile.

VIA custom-key indices 0–6 belong to the upstream Redragon lighting/Win-lock controls. Indices 7–9 select/toggle OS mode; 10–12 select/toggle input profile. Keep their order synchronized in both copies of the definition JSON. `CUSTOM(10)` is `0x7E0A` in this pinned fork.

Read-only 32-byte diagnostic requests use VIA `id_custom_get_value`:

| Request prefix | Response byte 3                   |
| -------------- | --------------------------------- |
| `08 4B 01`     | OS base layer: `0` or `2`         |
| `08 4B 02`     | Mac profile: `0` U.S., `1` EurKEY |

`build/via_probe_macos` prints these values; `--dump` double-reads keymaps and macro storage. Set `K630_USB_LOCATION` before running it. Treat dumps as private.

`CH555_EEPROM_LAYOUT=0x6303` identifies the five-layer storage layout. Changing layer count shifts the macro region; bump the layout identifier and document reset/migration behavior. Keymap exports and the active OS/profile are separate. Version 0.4 adds Fn+I without changing the five-layer storage geometry.

## Before proposing firmware changes

Keep the Esc recovery stub and flash bounds intact. Build and run offline checks, then list hardware checks separately with exact candidate hashes in your private validation record. Do not label an unflashed rebuild as hardware tested. See [CONTRIBUTING.md](../CONTRIBUTING.md) for reporting and publication expectations.
