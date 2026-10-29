# Redragon K630W-RGB / Evision VS11K28A

Experimental 61-key ANSI port for a physically verified VS11K28A controller.
Uses the CH555/8051 backend in mdmower/qmk-8051, pinned by the companion project.
It is incompatible with Sonix K630 firmware and is not validated for all K630 revisions.

The `via` keymap provides five editable layers: Windows, Windows Fn, Mac,
Mac Fn/media, and Mac F1–F12. Fn+O selects and saves Windows/Mac mode.
Mac Caps is Hyper; Fn followed by Menu accesses F keys. Fn+WASD/HJKL arrows
remain available. Mac Fn+I selects and saves the U.S./EurKEY input profile;
the computer's input source must match. Release keys and wait three seconds
before disconnecting after a setting change.

Matrix: P4.0–P4.5 rows and P0/P3 columns. **Esc is P4.1/P0.0**, requiring the
companion `EVISION_ESC_ROW=1` patch. Holding physical Esc during USB connection
erases the boot marker and enters recovery; a successful flash is then required
to type again. Keep your own verified stock backup and a second keyboard.

QMK USB ID: `5244:0630`. Load `keymaps/via/redragon_k630_vs11k28a_via.json`
in usevia.app's Design tab as V3. Stock `320f:5000` firmware does not speak VIA.

See the [companion repository](https://github.com/ScreamXY/K630W-RGB_VIA)
for installation, compatibility, lighting shortcuts, limitations and validation.
There is no VIA lighting panel, mouse-key, tap-hold or one-shot support.
