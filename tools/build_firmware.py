#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Build and check the K630 firmware. Requires git, clang, Python 3 and Docker.

Creates an isolated source checkout under build/. Never accesses USB.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
COMMIT = "c0d819da636af990611e874d2342f3b56e480455"
UPSTREAM = "https://github.com/mdmower/qmk-8051.git"
KEYBOARD = "redragon/k630_vs11k28a"
LUFA_COMMIT = "549b97320d515bfca2f95c145a67bd13be968faa"


def run(args, **kwargs):
    return subprocess.run(args, check=True, **kwargs)


def package(tree):
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    shutil.copy2(tree / "redragon_k630_vs11k28a_via.bin", build / "k630-via-experimental.bin")
    names = ["k630-via-experimental.bin", "evision_flash.py", "evision_oem_image.py"]
    for name in names[1:]:
        shutil.copy2(tree / "util" / name, build / name)
    if platform.system() == "Darwin":
        common = ["clang", "-Wall", "-Wextra", "-Werror", "-framework", "IOKit", "-framework", "CoreFoundation"]
        run(common + ["-dynamiclib", str(ROOT / "tools/k630_hid_macos.c"), "-o", str(build / "libk630_hid.dylib")])
        names.append("libk630_hid.dylib")
        for tool in ("list_usb_macos", "evision_backup_macos", "via_probe_macos"):
            run(common + ["-framework", "CoreGraphics", str(ROOT / f"tools/{tool}.c"), "-o", str(build / tool)])
            names.append(tool)
        run([str(build / "evision_backup_macos"), "--self-test"])
    manifest = {
        "recorded_at": datetime.now(timezone.utc).isoformat(), "upstream": UPSTREAM,
        "commit": COMMIT, "lufa_commit": LUFA_COMMIT, "keyboard": KEYBOARD,
        "usb": "5244:0630", "serial_source": "runtime chip ID (upstream default)",
        "esc_recovery": "P4.1/P0.0; hold Esc while plugging in", "hardware_tested": False,
        "files": {name: hashlib.sha256((build / name).read_bytes()).hexdigest() for name in names},
    }
    (build / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--package-only", action="store_true",
                        help="repackage native helpers and the manifest from an existing firmware build")
    args = parser.parse_args()
    tree = ROOT / "build/qmk-8051"
    if args.package_only:
        if not (tree / "redragon_k630_vs11k28a_via.bin").exists():
            raise SystemExit("No firmware build found; run without --package-only first.")
        package(tree)
        return
    tree.mkdir(parents=True, exist_ok=True)
    if not (tree / ".git").exists():
        if any(tree.iterdir()):
            raise SystemExit("Build checkout is nonempty and not a Git repository; refusing.")
        run(["git", "init", str(tree)])
        run(["git", "-C", str(tree), "fetch", "--depth", "1", UPSTREAM, COMMIT])
        run(["git", "-C", str(tree), "checkout", "--detach", "FETCH_HEAD"])
    head = subprocess.check_output(["git", "-C", str(tree), "rev-parse", "HEAD"], text=True).strip()
    if head != COMMIT:
        raise SystemExit("Source checkout differs from pinned commit; refusing.")
    patch = str(ROOT / "patches/k630-8051.patch")
    check = subprocess.run(["git", "-C", str(tree), "apply", "--check", patch], capture_output=True)
    if check.returncode == 0:
        run(["git", "-C", str(tree), "apply", patch])
    else:
        run(["git", "-C", str(tree), "apply", "--reverse", "--check", patch])
    run(["git", "-C", str(tree), "submodule", "update", "--init", "lib/lufa"])
    lufa = subprocess.check_output(["git", "-C", str(tree / "lib/lufa"), "rev-parse", "HEAD"], text=True).strip()
    if lufa != LUFA_COMMIT:
        raise SystemExit("LUFA differs from pinned commit; refusing.")
    shutil.copytree(ROOT / "port/keyboards/redragon/k630_vs11k28a",
                    tree / "keyboards/redragon/k630_vs11k28a", dirs_exist_ok=True)
    env = dict(os.environ, DOCKER_DEFAULT_PLATFORM="linux/amd64")
    log = ROOT / "build/build.log"
    print(f"Building {KEYBOARD}; log: {log}", flush=True)
    with log.open("w") as out:
        run([sys.executable, "-I", "util/ch555_build.py", KEYBOARD, "via"],
            cwd=tree, env=env, stdout=out, stderr=subprocess.STDOUT)
    package(tree)
    run([sys.executable, "-m", "unittest", "discover", "-s", str(ROOT / "tools"), "-p", "test_*.py", "-v"])
    print("Candidate packaged and checked. Nothing flashed.")


if __name__ == "__main__":
    main()
