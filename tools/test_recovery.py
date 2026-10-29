#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Offline checks for public tooling's recovery and device-selection guards."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import flash_macos
import prepare_backup as recovery


class RecoveryChecks(unittest.TestCase):
    def test_unknown_stock_is_rejected_even_with_valid_marker(self):
        with self.assertRaises(ValueError):
            recovery.validate_stock(bytes(65535))
        data = bytearray(65536)
        with self.assertRaises(ValueError):
            recovery.validate_stock(data)
        data[0x27c0:0x27c4] = recovery.MARKER
        with self.assertRaisesRegex(ValueError, "Unrecognized bootloader"):
            recovery.validate_stock(data)

    def test_local_record_rejects_modified_files_and_inexact_slice(self):
        # Fixture is not firmware. Mock only hardware validation to isolate
        # hash/slice checks independently from the optional real-ROM test.
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            full, app, path = root / "full.bin", root / "app.bin", root / "record.json"
            full.write_bytes(b"full backup fixture")
            app.write_bytes(b"app fixture")
            record = {"full_path": str(full), "app_path": str(app),
                      "full_sha256": recovery.digest(full.read_bytes()),
                      "app_sha256": recovery.digest(app.read_bytes()), "usb_location": "0x1234"}
            path.write_text(json.dumps(record))
            with patch.object(recovery, "validate_stock", return_value=b"app fixture"):
                self.assertEqual(recovery.validate_record(path), (record, app))
                for target in (full, app):
                    original = target.read_bytes()
                    target.write_bytes(original + b"changed")
                    with self.assertRaisesRegex(ValueError, "changed since registration"):
                        recovery.validate_record(path)
                    target.write_bytes(original)
                for location in ("0", "-1", "0x100000000"):
                    record["usb_location"] = location
                    path.write_text(json.dumps(record))
                    with self.assertRaisesRegex(ValueError, "Invalid saved USB location"):
                        recovery.validate_record(path)
            record["usb_location"] = "0x1234"
            path.write_text(json.dumps(record))
            with patch.object(recovery, "validate_stock", return_value=b"different app"):
                with self.assertRaisesRegex(ValueError, "not the exact slice"):
                    recovery.validate_record(path)

    def test_manifest_rejects_missing_files_paths_and_changed_content(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            build = root / "build"
            build.mkdir()
            names = ("k630-via-experimental.bin", "evision_flash.py", "evision_oem_image.py")
            for name in names:
                (build / name).write_bytes(b"fixture")
            files = {name: recovery.digest(b"fixture") for name in names}
            invalid = [{}, {**files, "../outside.py": recovery.digest(b"fixture")},
                       {**files, "evision_flash.py": "0" * 64}]
            with patch.object(flash_macos, "ROOT", root), patch.object(flash_macos, "load_upstream") as loader:
                for entries in invalid:
                    (build / "manifest.json").write_text(json.dumps({"files": entries}))
                    with self.assertRaises(SystemExit):
                        flash_macos.validate_bundle("install", require_backup=False)
                loader.assert_not_called()

    def test_restore_cannot_skip_backup_validation(self):
        with self.assertRaisesRegex(ValueError, "verified backup"):
            flash_macos.validate_bundle("restore", require_backup=False)

    def test_native_location_parser_has_no_default_and_rejects_bad_values(self):
        with tempfile.TemporaryDirectory() as td:
            source, binary = Path(td) / "location.c", Path(td) / "location"
            source.write_text('#include "device_location.h"\nint main(void) { return k630_location() ? 0 : 1; }\n')
            subprocess.run(["clang", "-Wall", "-Wextra", "-Werror", "-I", str(recovery.ROOT / "tools"),
                            str(source), "-o", str(binary)], check=True)
            env = dict(os.environ)
            env.pop("K630_USB_LOCATION", None)
            self.assertEqual(subprocess.run([str(binary)], env=env).returncode, 1)
            for value in ("", "0", "-1", "+1", "0x100000000", "0x1234junk", "999999999999999999999999999"):
                env["K630_USB_LOCATION"] = value
                self.assertEqual(subprocess.run([str(binary)], env=env).returncode, 1, value)
            for value in ("0x1234", "4294967295", "1"):
                env["K630_USB_LOCATION"] = value
                self.assertEqual(subprocess.run([str(binary)], env=env).returncode, 0, value)


if __name__ == "__main__":
    unittest.main(verbosity=2)
