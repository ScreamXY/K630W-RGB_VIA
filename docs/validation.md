# Validation record

## Physical prototype, 2026-10-29

One VS11K28A K630W-RGB was used for development. Its private stock backup, serial, USB location and raw logs are retained locally and are not distributed.

The installed v0.4.0 prototype passed:

- Two identical full stock-flash reads before the first installation; all 61 ROM key positions matched the port.
- App-range, vector and Esc-recovery checks, a successful bootloader checksum, and QMK/VIA enumeration after flashing.
- Physical Esc recovery across successive firmware updates.
- Basic typing, Fn+WASD arrows and RGB lighting.
- VIA connection in Chrome, live remapping, and remap persistence after normal USB reconnect.
- Fn+O Windows/Mac switching in both directions, Mac Command shortcuts, and persistence of Mac mode.
- Mac Caps/Hyper, Fn+Menu+1/= as F1/F12, Fn+−/= volume, and arrows together with the five-layer arrangement.
- All six umlauts with matching U.S. and EurKEY v1.2 profiles, physical Fn+I switching, and EurKEY behavior after normal USB reconnect.
- Readback of all 480 stored matrix positions, unchanged macro storage, Mac mode and EurKEY profile after reconnect.

The original firmware/transport suite had 13 passing checks. Independent macOS layout-table tests also produced all six umlauts in both profiles. Hardware behavior was confirmed by the keyboard owner; keymaps and selected modes were independently read back over VIA.

## Public source preparation

The public source keeps the v0.4.0 keyboard behavior. Publication changes remove the hardcoded development-unit serial and local USB/backup assumptions, add per-device recovery registration, and make offline tests runnable without a private ROM dump.

The public candidate is built from the pinned upstream revision and is checked offline. Its binary differs from the installed prototype, including use of upstream's runtime chip-ID serial. **The freshly packaged public candidate and revised host tools have not been used to reflash the physical keyboard.** A successful build is not an additional hardware-flash test. Read-only host-tool checks do not change the firmware or its settings.

Publication checks passed on 2026-10-29: a fresh pinned-source Docker build, all 18 offline checks with the private stock-ROM comparison enabled, the native backup framing self-test, and both macOS layout translations. The revised native tools also read the installed firmware version, five layers, Mac mode and saved EurKEY profile without modifying settings. The source snapshot passed a Gitleaks credential scan and explicit checks for local identifiers and paths.

The local manifest records the hash of each newly built artifact and marks it `hardware_tested: false`. Generated binaries, personal serials and raw evidence are not part of this source publication.

## Outstanding hardware checks

- A complete physical key sweep, beyond matrix/ROM comparison and the exercised keys.
- Macro recording/playback, including edge cases.
- Suspend/wake and long-duration behavior across hosts/hubs.
- Indicator behavior and measured current draw at all brightness settings.
- Physical restoration of the original stock app and its saved settings.
- Additional boards and PCB revisions.

Neither the build checks nor the successful prototype establish compatibility for another controller or firmware family. New hardware reports should follow [CONTRIBUTING.md](../CONTRIBUTING.md).
