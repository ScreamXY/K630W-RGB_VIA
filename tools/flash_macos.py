#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Reviewed bundle, locally verified recovery backup and bound USB port; dry-run by default.

Uses the pinned/patched upstream flasher for all image validation and the flash
transaction. The local C transport supplies only macOS HID access.
"""
import argparse
import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path
from types import SimpleNamespace
import time

from prepare_backup import DEFAULT_RECORD, validate_record

ROOT = Path(__file__).resolve().parents[1]
IDS = {(0x320f, 0x5000): 1, (0x0c45, 0x7687): 2, (0x5244, 0x0630): 3}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_upstream():
    path = ROOT / "build/evision_flash.py"
    spec = importlib.util.spec_from_file_location("evision_flash", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def validate_bundle(command, require_backup=True):
    manifest = json.loads((ROOT / "build/manifest.json").read_text())
    required = {"k630-via-experimental.bin", "evision_flash.py", "evision_oem_image.py"}
    if require_backup:
        required.add("libk630_hid.dylib")
    if not required.issubset(manifest["files"]):
        raise SystemExit("Bundle manifest is missing required files")
    for name, digest in manifest["files"].items():
        path = ROOT / "build" / name
        if path.parent != ROOT / "build" or sha(path) != digest:
            raise SystemExit(f"Bundle integrity failed: {name}")
    f = load_upstream()
    stock = None
    if require_backup:
        try:
            record, stock = validate_record()
        except (OSError, ValueError, KeyError) as error:
            raise SystemExit(f"Verified local recovery backup required: {error}") from error
        f.KNOWN[record["app_sha256"]] = "Locally verified stock recovery image"
        f.load_image(str(stock))
    elif command == "restore":
        raise ValueError("Restoration always requires a verified backup")
    candidate = ROOT / "build/k630-via-experimental.bin"
    _, info = f.load_image(str(candidate))
    data = candidate.read_bytes()
    off = info["entry"] - f.APP_BASE
    if data[off:off + len(f.ESC_CHECK_ROW1)] != f.ESC_CHECK_ROW1:
        raise SystemExit("K630 recovery check does not use P4.1; refusing.")
    if info["end"] >= 0xdf40:
        raise SystemExit("Application overlaps VIA settings storage; refusing.")
    return f, stock if command == "restore" else candidate


class MacTransport:
    def __init__(self, location):
        if not 0 < location <= 0xffffffff:
            raise ValueError("Invalid USB location")
        self.port = f"macOS USB location 0x{location:08x}"
        os.environ["K630_USB_LOCATION"] = hex(location)
        self.lib = ctypes.CDLL(str(ROOT / "build/libk630_hid.dylib"))
        for name in ("k630_present", "k630_open"):
            fn = getattr(self.lib, name)
            fn.argtypes = [ctypes.c_int]
            fn.restype = ctypes.c_int
        self.lib.k630_close.argtypes = []
        self.lib.k630_close.restype = None
        for name in ("k630_feature_write", "k630_feature_read", "k630_enter"):
            fn = getattr(self.lib, name)
            fn.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
            fn.restype = ctypes.c_int

    def present(self, mode):
        result = self.lib.k630_present(mode)
        if result < 0:
            raise SystemExit("Ambiguous or inaccessible keyboard interface; refusing.")
        return result == 1

    def iface(self, vid, pid):
        mode = IDS.get((vid, pid))
        return (mode, self.port) if mode and self.present(mode) else None

    def mode(self):
        found = [name for n, name in ((1, "app"), (2, "bootloader"), (3, "QMK")) if self.present(n)]
        if len(found) > 1:
            raise SystemExit("More than one firmware mode present at the bound USB port; refusing.")
        return found[0] if found else None

    def status(self, unused=None):
        print(f"{self.port}: {self.mode() or 'not found'}")

    def open(self, node):
        code = self.lib.k630_open(node)
        if code != 1:
            raise OSError(f"HID open failed: 0x{code & 0xffffffff:08x}")
        return node

    def close(self, unused):
        self.lib.k630_close()

    def write(self, name, data):
        if len(data) != 64:
            raise ValueError("Expected exactly 64 report bytes")
        buf = (ctypes.c_ubyte * 64).from_buffer_copy(data)
        result = getattr(self.lib, name)(buf, 64)
        if result:
            raise OSError(f"{name}: 0x{result & 0xffffffff:08x}")

    def get_feature(self, unused):
        buf = (ctypes.c_ubyte * 64)()
        result = self.lib.k630_feature_read(buf, 64)
        if result:
            raise OSError(f"GET_FEATURE: 0x{result & 0xffffffff:08x}")
        return bytes(buf)

    def wait_usb_at(self, port, ids, timeout):
        if port != self.port:
            raise ValueError("Unexpected USB port")
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            for pair in ids:
                if self.iface(*pair):
                    return pair
            time.sleep(0.25)
        return None

    def bind(self, f):
        f.iface1_node = self.iface
        f.usb_port = lambda port: port
        f.mode = self.mode
        f.do_status = self.status
        f.open_node = self.open
        f.close_node = self.close
        f.set_feature = lambda fd, data: self.write("k630_feature_write", data)
        f.get_feature = self.get_feature
        f.write_report = lambda fd, data: self.write("k630_enter", data)
        f.wait_usb_at = self.wait_usb_at
        f.wait_access = lambda node, timeout: self.present(node)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("status", "install", "restore"))
    parser.add_argument("--send", action="store_true", help="actually enter/flash; otherwise dry-run")
    parser.add_argument("--location", type=lambda s: int(s, 0),
                        help="USB location; defaults to K630_USB_LOCATION or the registered backup")
    a = parser.parse_args()
    record = json.loads(DEFAULT_RECORD.read_text()) if DEFAULT_RECORD.exists() else {}
    location = a.location or int(os.environ.get("K630_USB_LOCATION", record.get("usb_location", "0")), 0)
    if not location:
        parser.error("Select --location or register a recovery backup first.")
    if a.command != "status" and not record:
        parser.error("Register a verified stock backup with tools/prepare_backup.py first.")
    if a.command != "status" and location != int(record.get("usb_location", "0"), 0):
        parser.error("USB location differs from the recovery record; "
                     "verify the device and update the local record deliberately.")
    if a.command == "status":
        MacTransport(location).status()
        return
    # Both the candidate and the untouched recovery image are validated BEFORE
    # the destructive bootloader-entry command is even reachable.
    f, image = validate_bundle(a.command)
    transport = MacTransport(location)
    transport.bind(f)
    mode = transport.mode()
    if mode not in ("app", "bootloader", "QMK"):
        raise SystemExit("Keyboard missing from its original USB port.")
    args = SimpleNamespace(send=a.send, image=str(image), settings=None, poll=False,
                           gap=0.03, expect=(0x320f, 0x5000) if a.command == "restore" else (0x5244, 0x0630))
    print("Recovery backup verified. Firmware writes are confined to the app region;")
    print("entering bootloader erases its marker and requires a successful flash to type again.")
    if mode == "QMK":
        raise SystemExit("Unplug the keyboard, hold Esc while plugging it in, then run this command again.")
    if mode == "app":
        if a.command == "restore":
            raise SystemExit("Already running stock firmware; no restore needed.")
        f.do_enter(args)
    f.do_flash(args)


if __name__ == "__main__":
    main()
