#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Validate a matching stock backup and register local recovery data. No USB access."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
# Firmware-family fingerprint, excluding the marker page. Not a device serial.
BOOT_SHA = "41f96f863d081044a847ae99d94f1b02cdf1b85b4f3f6062a92e4737ee76af18"
APP_START, APP_END = 0x2800, 0xefc0
MARKER = bytes.fromhex("aa 55 33 cc")
DEFAULT_RECORD = ROOT / "backups/recovery.json"
BOARD = ROOT / "port/keyboards/redragon/k630_vs11k28a/keyboard.json"

# Stock ROM default key records: 3 bytes per matrix position, by keyboard.json label.
ROM_KEY_TABLE = 0x6210
KEY_USAGES = {chr(ord("A") + i): 0x04 + i for i in range(26)}
KEY_USAGES.update({str(i): 0x1d + i for i in range(1, 10)})
KEY_USAGES.update({
    "0": 0x27, "Esc": 0x29, "-": 0x2d, "=": 0x2e, "Backspace": 0x2a,
    "Tab": 0x2b, "[": 0x2f, "]": 0x30, "Backslash": 0x31, "CapsLock": 0x39,
    ";": 0x33, "Quote": 0x34, "Enter": 0x28, "Comma": 0x36, "Dot": 0x37,
    "Slash": 0x38, "Space": 0x2c, "Menu": 0x65,
})
MODIFIER_BITS = {"LeftCtrl": 1, "LeftShift": 2, "LeftAlt": 4, "LeftGUI": 8,
                 "RightCtrl": 16, "RightShift": 32, "RightAlt": 64}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def rom_key_offset(row, col):
    return ROM_KEY_TABLE + (col * 8 + row) * 3


def expected_rom_key(label):
    if label in MODIFIER_BITS:
        return bytes([2, 1, MODIFIER_BITS[label]])
    if label == "Fn":
        return bytes([5, 2, 2])
    return bytes([2, 2, KEY_USAGES[label]])


def validate_stock(data):
    if len(data) != 65536 or data[0x27c0:0x27c4] != MARKER:
        raise ValueError("Expected a complete stock 64 KiB dump with its boot marker.")
    if digest(data[:0x27c0]) != BOOT_SHA:
        raise ValueError("Unrecognized bootloader. Do not bypass this check; investigate the hardware revision.")
    # Stock ROM matrix, independent of a user's saved keymaps/macros.
    board = json.loads(BOARD.read_text())
    for key in board["layouts"]["LAYOUT_60_ansi"]["layout"]:
        offset = rom_key_offset(*key["matrix"])
        if data[offset:offset + 3] != expected_rom_key(key["label"]):
            raise ValueError(f"Stock ROM matrix does not match this port: {key['label']}.")
    app = data[APP_START:APP_END]
    for offset in [0] + [0x800 + 3 * i for i in range(13)]:
        target = int.from_bytes(app[offset + 1:offset + 3], "big")
        if app[offset] != 2 or not APP_START <= target < APP_END:
            raise ValueError("Invalid stock app entry/interrupt table.")
    return app


def validate_record(path=DEFAULT_RECORD):
    record = json.loads(Path(path).read_text())
    full = Path(record["full_path"])
    app = Path(record["app_path"])
    data = full.read_bytes()
    image = app.read_bytes()
    if digest(data) != record["full_sha256"] or digest(image) != record["app_sha256"]:
        raise ValueError("Recovery backup changed since registration.")
    if validate_stock(data) != image:
        raise ValueError("Recovery app is not the exact slice of the stock backup.")
    location = int(record["usb_location"], 0)
    if not 0 < location <= 0xffffffff:
        raise ValueError("Invalid saved USB location.")
    return record, app


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("backup", type=Path)
    parser.add_argument("--location", required=True, type=lambda s: int(s, 0))
    parser.add_argument("--confirm-controller", required=True, choices=["VS11K28A"],
                        help="confirm the MCU marking physically, not from the USB ID")
    args = parser.parse_args()
    if not 0 < args.location <= 0xffffffff:
        parser.error("--location must be a nonzero 32-bit USB location")
    data = args.backup.read_bytes()
    app = validate_stock(data)
    dest = ROOT / "backups/stock-app.bin"
    DEFAULT_RECORD.parent.mkdir(exist_ok=True)
    if dest.exists() or DEFAULT_RECORD.exists():
        parser.error("Recovery files already exist; preserve them before registering another device.")
    with dest.open("xb") as output:
        output.write(app)
    record = {
        "full_path": str(args.backup.resolve()), "app_path": str(dest.resolve()),
        "full_sha256": digest(data), "app_sha256": digest(app), "usb_location": hex(args.location),
    }
    with DEFAULT_RECORD.open("x") as output:
        json.dump(record, output, indent=2)
        output.write("\n")
    print("Validated bootloader, all 61 ROM matrix positions and app vectors. Registered local recovery backup.")


if __name__ == "__main__":
    main()
