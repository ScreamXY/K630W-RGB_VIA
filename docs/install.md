# Install, update and recover on macOS

This guide is for the **VS11K28A K630W-RGB** described in [hardware.md](hardware.md). Do not use it for a Sonix or unknown controller. A matching USB ID is insufficient. The backup validator deliberately rejects unfamiliar bootloaders or ROM matrices; investigate a failure instead of disabling the checks.

Keep a second keyboard available and preserve your backups outside this working folder. The supplied macOS workflow has been exercised on one device. Restoration has been validated offline but has not been physically tested.

## 1. Build before entering update mode

Install Python 3, Git, Xcode Command Line Tools and Docker Desktop. Confirm `docker info` succeeds, then run from the repository root:

```sh
python3 tools/build_firmware.py
```

This fetches the pinned source and LUFA revision, builds the Docker compiler image if missing, compiles the firmware, builds native tools, and runs offline checks. It makes no USB requests. Apple Silicon builds use `linux/amd64` for the upstream SDCC toolchain. Outputs and the build log are in ignored `build/`.

The offline suite normally skips the private stock-ROM comparison until you supply your own backup. Build success does not establish hardware compatibility.

## 2. Identify the USB location

Connect the reassembled keyboard normally, without holding Esc. Run:

```sh
build/list_usb_macos
```

This only enumerates candidate HID interfaces and prints USB IDs and location IDs. It does not open devices, send reports or print serials. Multiple lines at the same location can belong to one composite keyboard. Disconnect other keyboards with the same candidate IDs while making the stock backup.

Set `K630_USB_LOCATION` to the location shown for your keyboard. Replace the placeholder below with the actual hexadecimal value; do not copy an address from somebody else's setup:

```sh
export K630_USB_LOCATION='<your USB location, starting with 0x>'
```

Use the same physical USB port/hub for backup and flashing. Location binding distinguishes ports, not individual serial numbers: do not substitute another keyboard at that location.

## 3. Make and validate your own stock backup

This step requires the original stock firmware (`320f:5000`). It cannot read stock firmware from a keyboard already running QMK or in the bootloader.

```sh
mkdir -p backups
build/evision_backup_macos --self-test
build/evision_backup_macos
```

The second command checks read-only packet framing offline. The third is a dry run: it identifies the stock vendor interface without opening it. For actual read access, macOS may require **Privacy & Security → Input Monitoring** permission for your terminal application. Restart that application after granting access if necessary.

Then request two complete reads:

```sh
build/evision_backup_macos --send --out backups/stock-full64k.bin
```

Here `--send` sends **read requests only**. The reader has no bootloader-entry or flash-write command. It saves a new 65,536-byte file only if both reads match and the marker is correct. It refuses to overwrite an existing file. Do not type or change keyboard settings during the reads.

Register the backup, after physically confirming the controller marking:

```sh
python3 tools/prepare_backup.py backups/stock-full64k.bin \
  --location "$K630_USB_LOCATION" --confirm-controller VS11K28A
```

This offline operation checks the bootloader fingerprint, all 61 ROM matrix positions, boot marker and app vectors. It extracts the untouched recovery app to `backups/stock-app.bin` and writes `backups/recovery.json` with local paths, hashes and USB location. Both remain private and ignored by Git. Copy the entire backup folder to durable storage before proceeding. The record uses absolute local paths; if you later move the backups, update `full_path` and `app_path` to their new locations without changing the hashes.

For the optional real-ROM test:

```sh
K630_STOCK_BACKUP=backups/stock-full64k.bin python3 -m unittest discover -s tools -p 'test_*.py' -v
```

A failed compatibility check means this port is not established for your unit. Do not register somebody else's backup, edit hashes to bypass validation, or write a full 64 KiB dump using the app flasher.

## 4. Check, then install

Inspect `build/manifest.json` and run:

```sh
python3 tools/flash_macos.py status
python3 tools/flash_macos.py install
```

`install` without `--send` is a dry run. It checks bundle hashes, the verified local recovery record, application bounds and the correct Esc recovery stub before any destructive command is reachable. It can still open the HID interface; it does not send flash or bootloader-entry commands.

For the first installation from stock, the following command deliberately enters the bootloader and flashes:

```sh
python3 tools/flash_macos.py install --send
```

The stock-to-bootloader command **erases the boot marker**. The keyboard stops typing until a flash succeeds. Keep it connected while the flasher checks the bootloader handshake, writes the application, checks its checksum and waits for `5244:0630` at the same USB location. Do not treat a USB reset-transfer error alone as failure: the tool independently checks re-enumeration.

Once complete, load the VIA definition as described in the [README](../README.md#use-via-after-installation). Fresh firmware starts in Windows mode and U.S. profile. Press Fn+O for Mac; set the keyboard input profile and host input source to match.

## 5. Future updates from QMK

Export your VIA layout and finish building/checking the next candidate **before** entering update mode. Confirm the registered backup and same USB location are available. In QMK mode, `install` validates the bundle and then asks for physical Esc recovery instead of sending an entry command.

1. Unplug the K630.
2. Hold physical Esc, reconnect USB, then release Esc.
3. Run `python3 tools/flash_macos.py status`; it should show `bootloader`.
4. Run the dry run, then `python3 tools/flash_macos.py install --send`.

**Holding Esc erases the boot marker.** Reconnecting normally cannot cancel update mode; complete a flash to resume typing. Flashing erases VIA settings, so restore a matching five-layer export afterward and reselect OS/input profile. Wait three seconds after changes before disconnecting.

## Recovery and stock restoration

If a write fails and the bootloader remains present, keep the validated backup and complete build bundle intact. Check `status`, then retry the same validated installation or restore your local stock app:

```sh
python3 tools/flash_macos.py restore
python3 tools/flash_macos.py restore --send
```

The first command is a dry run. For restoration while QMK is running, use the physical Esc sequence first. The tool refuses to restore while stock is already running. It validates the registered backup before accepting it as a known recovery image and writes the app slice, not bootloader code. Expected stock identity is `320f:5000`.

Restoration retains the saved stock profiles contained in your original app backup. It does not overwrite the lighting data region outside that app slice. A full factory-state restoration is not claimed, and physical stock restoration remains untested.

If the device no longer enumerates even as a bootloader, USB-only recovery is not established by this project. Report the hardware revision and sanitized diagnostics; do not try firmware for another controller.

## Troubleshooting

- **Docker application open but engine unavailable:** wait until `docker info` succeeds before building.
- **HID permission/open error:** check Input Monitoring for the application running the tools, reconnect normally, and retry status. Do not use `sudo` as a substitute for macOS privacy permission.
- **USB location changed:** enumerate again. After verifying the same physical keyboard and its preserved backup, update only `usb_location` in your local `backups/recovery.json`. Do not weaken the hash or hardware checks.
- **VIA cannot see the device:** confirm QMK identity `5244:0630`, use Chrome with WebHID, load the V3 definition, then authorize again. Embedded Electron browsers may hide VIA's WebHID controls.
- **Umlauts are wrong:** match the Mac input source and saved keyboard profile; see [keymap troubleshooting](keymap.md#us-and-eurkey-profiles).
- **Probe/status reads:** `K630_USB_LOCATION` is required for `build/via_probe_macos`; the Python flasher can read the location from `backups/recovery.json`.

`build/via_probe_macos --dump` reads keymaps and macros twice and compares them. Keep its output private: macros can contain sensitive text. This diagnostic format is not a VIA Save + Load export.
