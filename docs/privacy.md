# Private files and public source

The public repository contains source code, tests, the generic VIA definition, documentation and upstream license/patch material. It intentionally excludes:

- `backups/`: proprietary stock ROM, saved profiles, macros, recovery hashes and absolute paths.
- `build/`: generated firmware, executables, native libraries, manifests, downloaded upstream checkout and logs.
- `evidence/`: raw USB reports, hardware logs, device serials, live keymaps and machine paths.
- `local/`, `.k630-local.json`, environment files and Python caches.

These paths are ignored by Git. `.gitignore` is not a privacy boundary if someone force-adds files; inspect staged content before publishing. Avoid placing exports or screenshots elsewhere in the repository. Keyboard macros can contain arbitrary text, including passwords or private commands.

The public code has no embedded device serial, personal filesystem path, fixed physical USB location or private stock-app checksum. Hardware IDs, matrix addresses, upstream commit IDs and the supported bootloader-code fingerprint are intentional technical information. A fresh firmware build uses the connected chip's ID as its runtime USB serial; that serial may appear in local USB diagnostics even though it is not baked into public source.

`tools/prepare_backup.py` creates a local recovery record from each user's verified backup. Keep both the full dump and extracted app private, and retain a durable copy outside the repository. Do not attach stock dumps or unredacted VIA macro dumps to public issues.

Before publishing, scan the commit with a secret scanner such as Gitleaks and search it for your own serial, USB location, username and backup hashes. Such checks reduce accidental disclosure; they do not promise that every future contribution or generated file is free of sensitive data. Contributors should also use their preferred public or GitHub noreply commit email.
