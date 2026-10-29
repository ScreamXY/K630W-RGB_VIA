#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Offline firmware/transport checks. Set K630_STOCK_BACKUP for the optional ROM comparison."""
import contextlib
import importlib.util
import io
import json
import os
import re
import subprocess
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from flash_macos import ROOT, validate_bundle
from prepare_backup import rom_key_offset, validate_stock


class PortChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.f, cls.path = validate_bundle("install", require_backup=False)
        backup = os.environ.get("K630_STOCK_BACKUP")
        cls.stock = Path(backup).read_bytes() if backup else None
        cls.bin = cls.path.read_bytes()

    def test_all_physical_keys_match_stock_firmware(self):
        if self.stock is None:
            self.skipTest("Set K630_STOCK_BACKUP to compare your private stock ROM")
        j = json.loads((ROOT / "port/keyboards/redragon/k630_vs11k28a/keyboard.json").read_text())
        layout = j["layouts"]["LAYOUT_60_ansi"]["layout"]
        self.assertEqual(len(layout), 61)
        self.assertEqual(len({tuple(k["matrix"]) for k in layout}), 61)
        # Compare ROM defaults, never user-remappable saved profiles.
        validate_stock(self.stock)
        # Every populated position is checked: corrupting any single record must fail.
        for key in layout:
            corrupt = bytearray(self.stock)
            corrupt[rom_key_offset(*key["matrix"]) + 2] ^= 0xff
            with self.assertRaisesRegex(ValueError, "does not match this port"):
                validate_stock(bytes(corrupt))

    def test_via_coordinates_and_ids_match_firmware(self):
        j = json.loads((ROOT / "port/keyboards/redragon/k630_vs11k28a/keyboard.json").read_text())
        v = json.loads((ROOT / "via/k630-vs11k28a.json").read_text())
        embedded = ROOT / "port/keyboards/redragon/k630_vs11k28a/keymaps/via/redragon_k630_vs11k28a_via.json"
        self.assertEqual(v, json.loads(embedded.read_text()))
        coords = [tuple(map(int, x.split(","))) for row in v["layouts"]["keymap"] for x in row if isinstance(x, str)]
        self.assertEqual(coords, [tuple(k["matrix"]) for k in j["layouts"]["LAYOUT_60_ansi"]["layout"]])
        self.assertEqual((v["vendorId"], v["productId"]), (j["usb"]["vid"], j["usb"]["pid"]))
        self.assertEqual(v["matrix"], {"rows": 6, "cols": 16})
        # Actual USB device descriptor in the linked binary, not just its JSON.
        self.assertTrue(bytes.fromhex("44 52 30 06 40 00") in self.bin,
                        "USB descriptor must contain 5244:0630, bcdDevice 0.4.0")

    def test_os_switch_handler(self):
        with tempfile.TemporaryDirectory() as td:
            binary = str(Path(td) / "test_os_mode")
            board = ROOT / "port/keyboards/redragon/k630_vs11k28a"
            subprocess.run(["clang", "-Wall", "-Wextra", "-Werror", "-DVIA_ENABLE",
                            "-I" + str(ROOT / "tools/test_stubs"), "-I" + str(board),
                            str(ROOT / "tools/test_os_mode.c"), str(board / "os_mode.c"),
                            "-o", binary], check=True)
            subprocess.run([binary], check=True, capture_output=True)

    def test_mac_umlaut_handler(self):
        with tempfile.TemporaryDirectory() as td:
            binary = str(Path(td) / "test_umlaut")
            board = ROOT / "port/keyboards/redragon/k630_vs11k28a"
            subprocess.run(["clang", "-Wall", "-Wextra", "-Werror",
                            "-I" + str(ROOT / "tools/test_stubs"), "-I" + str(board),
                            str(ROOT / "tools/test_umlaut.c"), str(board / "umlaut.c"),
                            "-o", binary], check=True)
            subprocess.run([binary], check=True, capture_output=True)

    def test_windows_mac_keymaps_preserve_arrows_and_switch_access(self):
        board = ROOT / "port/keyboards/redragon/k630_vs11k28a"
        text = (board / "keymaps/default/keymap.c").read_text()
        layers = re.findall(r"\[(K630_\w+)\] = LAYOUT_60_ansi\((.*?)\n    \)", text, re.S)
        self.assertEqual([name for name, _ in layers],
                         ["K630_WINDOWS", "K630_WINDOWS_FN", "K630_MAC", "K630_MAC_FN", "K630_MAC_FKEYS"])
        layout = json.loads((board / "keyboard.json").read_text())["layouts"]["LAYOUT_60_ansi"]["layout"]
        maps = {}
        for name, body in layers:
            codes = [code.strip() for code in body.split(",")]
            self.assertEqual(len(codes), 61)
            maps[name] = {tuple(key["matrix"]): code for key, code in zip(layout, codes)}
        for name in ("K630_WINDOWS_FN", "K630_MAC_FN"):
            self.assertEqual([maps[name][p] for p in ((2, 2), (3, 1), (3, 2), (3, 3))],
                             ["KC_UP", "KC_LEFT", "KC_DOWN", "KC_RGHT"])
            self.assertEqual(maps[name][2, 9], "K630_OS_TOGGLE")
        self.assertEqual(maps["K630_WINDOWS"][5, 10], "MO(K630_WINDOWS_FN)")
        self.assertEqual(maps["K630_MAC"][5, 10], "MO(K630_MAC_FN)")
        self.assertEqual([maps["K630_MAC"][5, c] for c in (0, 1, 2, 9, 11)],
                         ["KC_LCTL", "KC_LALT", "KC_LGUI", "KC_RGUI", "KC_RALT"])
        self.assertEqual(maps["K630_MAC"][3, 0], "KC_HYPR")
        self.assertEqual(maps["K630_WINDOWS"][3, 0], "KC_CAPS")
        self.assertEqual(maps["K630_MAC_FN"][5, 11], "MO(K630_MAC_FKEYS)")
        self.assertEqual(maps["K630_MAC_FN"][2, 8], "K630_LAYOUT_TOGGLE")
        self.assertEqual(maps["K630_WINDOWS_FN"][2, 8], "_______")
        self.assertEqual(maps["K630_WINDOWS_FN"][5, 11], "_______")
        self.assertEqual([maps["K630_MAC_FN"][1, c] for c in range(1, 13)],
                         ["KC_F14", "KC_F15", "KC_MCTL", "KC_LPAD", "KC_BRID", "KC_BRIU",
                          "KC_MPRV", "KC_MPLY", "KC_MNXT", "KC_MUTE", "KC_VOLD", "KC_VOLU"])
        for c in range(1, 13):
            self.assertEqual(maps["K630_MAC_FKEYS"][1, c], "KC_F" + str(c))
            self.assertEqual(maps["K630_WINDOWS_FN"][1, c], "KC_F" + str(c))
        for pos, code in maps["K630_MAC_FKEYS"].items():
            if pos[0] != 1 or not 1 <= pos[1] <= 12:
                self.assertEqual(code, "_______", "Fn, Hyper and arrows must fall through")
        config = (board / "keymaps/via/config.h").read_text()
        self.assertIn("DYNAMIC_KEYMAP_LAYER_COUNT 5", config)
        self.assertIn("CH555_EEPROM_LAYOUT 0x6303", config)
        definition = json.loads((ROOT / "via/k630-vs11k28a.json").read_text())
        self.assertEqual(definition["customKeycodes"][7]["shortName"], "OS Tog")
        self.assertEqual([k["shortName"] for k in definition["customKeycodes"][10:]], ["US/Eur", "US Kbd", "EurKbd"])

    def test_recovery_row_changes_only_two_bit_operands(self):
        spec = importlib.util.spec_from_file_location("oem_image", ROOT / "build/evision_oem_image.py")
        m = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(m)
        old, oldret = m.esc_check(0x2803, 0x3027)
        new, newret = m.esc_check(0x2803, 0x3027, 1)
        changes = [(i, a, b) for i, (a, b) in enumerate(zip(old, new)) if a != b]
        self.assertEqual([(a, b) for i, a, b in changes], [(0xc0, 0xc1), (0xc0, 0xc1)])
        self.assertEqual([new[i - 1] for i, a, b in changes], [0xd2, 0x20])
        self.assertEqual(oldret, newret)
        self.assertEqual(self.bin[3:3 + len(new)], new)
        self.assertEqual(new[:-3], self.f.ESC_CHECK_ROW1)
        self.assertEqual(old[:-3], self.f.ESC_CHECK)

    def test_reject_full_flash_oversize_and_broken_recovery(self):
        broken = bytearray(self.bin)
        broken[3] ^= 1
        cases = [bytes(65536), self.bin + bytes(0x10000), bytes(broken)]
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "bad.bin"
            for data in cases:
                p.write_bytes(data)
                with self.assertRaises(SystemExit):
                    self.f.load_image(str(p))

    def flash_simulation(self, fail_command=None, reset_error=False, reappears=True):
        writes = []
        _, info = self.f.load_image(str(self.path))
        def send(fd, data):
            writes.append(data)
            if reset_error and len(writes) == info["packets"] + 5:
                raise OSError("USB disconnected during reset")
        def reply(fd):
            cmd = writes[-1][0]
            status = bytes(4) if cmd == fail_command else b"\xfa" * 4
            return bytes([cmd, 0xaa, 0x55, 0]) + status + bytes.fromhex("00 11 22 33") + bytes(52)
        args = SimpleNamespace(image=str(self.path), expect=(0x5244, 0x0630), settings=None,
                               poll=False, send=True, gap=0.03)
        with contextlib.ExitStack() as stack:
            replacements = dict(iface1_node=lambda *a: ("fake", "port"), usb_port=lambda x: x,
                                open_node=lambda x: 1, close_node=lambda x: None,
                                set_feature=send, get_feature=reply,
                                wait_usb_at=lambda *a: args.expect if reappears else None,
                                do_status=lambda a: None)
            for name, value in replacements.items():
                stack.enter_context(patch.object(self.f, name, value))
            stack.enter_context(patch.object(self.f.time, "sleep", lambda t: None))
            stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
            if fail_command or not reappears:
                with self.assertRaises(SystemExit):
                    self.f.do_flash(args)
            else:
                self.f.do_flash(args)
        return writes

    def test_transaction_bounds_and_exact_payload(self):
        writes = self.flash_simulation()
        padded, info = self.f.load_image(str(self.path))
        self.assertEqual([x[0] for x in writes[:3]], [1, 2, 3])
        self.assertEqual(int.from_bytes(writes[2][4:8], "little"), 0x2800)
        packets = int.from_bytes(writes[2][8:10], "little")
        self.assertEqual(packets, info["packets"])
        self.assertLessEqual(0x2800 + packets * 64, 0xefc0)
        self.assertEqual(b"".join(writes[3:3 + packets]), padded)
        self.assertEqual([x[0] for x in writes[-2:]], [4, 5])
        self.assertEqual(int.from_bytes(writes[-2][8:12], "little"), sum(padded))

    def test_bad_hello_never_erases(self):
        writes = self.flash_simulation(1)
        self.assertEqual(len(writes), 1)
        self.assertEqual(writes[0][0], 1)

    def test_failed_checksum_never_resets(self):
        writes = self.flash_simulation(4)
        _, info = self.f.load_image(str(self.path))
        self.assertEqual(len(writes), info["packets"] + 4)
        self.assertEqual(writes[-1][0], 4)

    def test_reset_disconnect_succeeds_only_after_reenumeration(self):
        writes = self.flash_simulation(reset_error=True)
        self.assertEqual(writes[-1][0], 5)

    def test_reset_disconnect_without_reenumeration_fails(self):
        writes = self.flash_simulation(reset_error=True, reappears=False)
        self.assertEqual(writes[-1][0], 5)

    def test_successful_reset_transfer_without_reenumeration_fails(self):
        self.flash_simulation(reappears=False)


if __name__ == "__main__":
    unittest.main(verbosity=2)
