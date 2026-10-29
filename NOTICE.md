# License and attribution

This project's source additions are provided under **GPL-2.0-or-later**. [LICENSE](LICENSE) contains GNU GPL version 2; source-file SPDX headers identify the “or later” option. Upstream files retain their own copyright and license notices.

The firmware builds on [mdmower/qmk-8051](https://github.com/mdmower/qmk-8051/tree/c0d819da636af990611e874d2342f3b56e480455), pinned at `c0d819da636af990611e874d2342f3b56e480455`. That project supplies the 8051/CH555 QMK implementation, Evision image/flash utilities, EEPROM support, Redragon lighting and common keyboard code. It in turn derives from QMK and its contributors. The build fetches QMK's LUFA revision `549b97320d515bfca2f95c145a67bd13be968faa`, whose notices remain in the fetched source tree.

Changes supplied by this project, **2026-10-29**, include:

- VS11K28A K630 matrix and LED-position definitions, board setup and five-layer VIA keymap.
- Saved Windows/Mac selection, Mac Hyper/media/F-key behavior, and saved U.S./EurKEY umlaut profiles.
- An upstream patch adding row-1 Esc recovery selection, K630 QMK-ID recognition, and verification of USB re-enumeration after a reset-transfer error.
- macOS HID backup/flash transport, local recovery validation, tests and documentation.

The macOS stock reader implements the read protocol documented in the pinned upstream `util/evision_dump.py`. The supplied patch modifies `platforms/mcs51/platform.mk`, `util/evision_oem_image.py` and `util/evision_flash.py`; it is intended to be applied to that exact revision.

No original Redragon firmware dump, proprietary vendor software or prebuilt firmware binary is distributed here. Users create and retain their own recovery backups locally. If distributing a compiled derivative, preserve the relevant licenses and provide corresponding source and build material as required by those licenses; this source-only repository does not replace those obligations.

Redragon and VIA names identify compatibility and interoperability. This community project is not affiliated with or endorsed by Redragon, QMK or VIA.
