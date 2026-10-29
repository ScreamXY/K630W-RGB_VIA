# Contributing

Please open an issue describing the problem or proposed change before attempting support for another hardware variant. The current compatibility claim is deliberately limited to the VS11K28A board described in [docs/hardware.md](docs/hardware.md).

For a hardware report, include the model, visible MCU marking and PCB revision, observed USB mode, firmware version, and a short reproduction. Redact serial labels, personal account details and host paths from photographs or diagnostics. Do not upload stock firmware dumps or VIA macro exports. A different bootloader fingerprint or matrix needs investigation; do not bypass the validator simply to make a flash proceed.

For code changes:

1. Use the pinned build workflow in [docs/development.md](docs/development.md).
2. Run the applicable offline checks and update documentation for changed behavior.
3. Keep both copies of the VIA definition synchronized.
4. Keep recovery pin selection, flash boundaries and validation before destructive commands intact.
5. Distinguish simulated checks, read-only device checks and actual hardware-flash tests in the pull request.
6. Inspect `git diff --cached` for personal paths, serials, dumps and credentials. Keep generated artifacts and recovery data ignored.

Tests require no keyboard. Physical flashing is a separate, deliberate step and is not expected for documentation-only contributions. Please preserve SPDX headers and upstream attribution when adapting code.
