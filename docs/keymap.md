# Layers and customization

## Five layers

| VIA layer | Purpose              | Access                                        |
| --------- | -------------------- | --------------------------------------------- |
| 0         | Windows base         | Fn+O or custom Windows mode key               |
| 1         | Windows Fn           | Hold Fn (`MO(1)` on layer 0)                  |
| 2         | Mac base             | Fn+O or custom Mac mode key                   |
| 3         | Mac media/navigation | Hold Fn (`MO(3)` on layer 2)                  |
| 4         | Mac F-key overlay    | Hold Fn first, then Menu (`MO(4)` on layer 3) |

All five layers are independently editable. Keep the Fn bindings and OS toggle accessible when editing. Layer 4 is transparent outside the number row, preserving arrows, Hyper, and mode switching. The Windows layers retain conventional modifiers, Caps Lock, and Fn+number-row F keys; the Mac-only additions do not affect them.

## Mac modifiers and Hyper

| Physical key      | Windows action | Mac action |
| ----------------- | -------------- | ---------- |
| Left Ctrl         | Control        | Control    |
| Left Win          | Windows        | Option     |
| Left Alt          | Alt            | Command    |
| Right Alt         | Alt            | Command    |
| Menu, right of Fn | Menu           | Option     |
| Right Ctrl        | Control        | Control    |
| Caps Lock         | Caps Lock      | Hyper      |

Hyper holds Ctrl+Shift+Option+Command together. It does not toggle Caps Lock. In VIA it can be assigned with **Any → `HYPR(KC_NO)`** (keycode `0x0F00`). For Hyper+F1, hold Caps, Fn, then Menu and press 1.

**Fn+O** toggles the persistent Windows/Mac base layer. The switch releases held keys and clears momentary layers to prevent modifiers or the previous Fn layer from remaining active. Fn+A and Fn+S remain Left and Down; they are not OS selectors.

## Mac media and F keys

| Key | Fn                              | Fn, then Menu |
| --- | ------------------------------- | ------------- |
| 1   | F14 / display dim shortcut      | F1            |
| 2   | F15 / display brighter shortcut | F2            |
| 3   | Mission Control                 | F3            |
| 4   | Launchpad                       | F4            |
| 5   | Brightness down                 | F5            |
| 6   | Brightness up                   | F6            |
| 7   | Previous track                  | F7            |
| 8   | Play/pause                      | F8            |
| 9   | Next track                      | F9            |
| 0   | Mute                            | F10           |
| −   | Volume down                     | F11           |
| =   | Volume up                       | F12           |

These are keyboard/consumer usages; their effect depends on macOS version, display support and shortcut assignments. Release Menu to return to the media row. Hold Fn **before** Menu, because Menu alone is Option in Mac mode.

## Navigation and lighting

Both OS modes provide these Fn shortcuts:

| Keys                         | Action                                    |
| ---------------------------- | ----------------------------------------- |
| W / A / S / D                | Up / Left / Down / Right                  |
| H / J / K / L                | Left / Down / Up / Right                  |
| Esc / Backspace              | Grave / Delete                            |
| ; / '                        | Home / End                                |
| . / /                        | Page Up / Page Down                       |
| P / [ / ] / Backslash        | Print Screen / Insert / Delete / Pause    |
| Q / E / C                    | Volume down / Volume up / Mute            |
| Left Alt / Space / Right Alt | Previous track / Play-pause / Next track  |
| Z / X                        | LED effect / LEDs on-off                  |
| R / T / Y / V                | LED hue / saturation / speed / brightness |

Hold Shift to step a lighting setting backwards. The initial LED intensity is reduced from the upstream default; actual current draw has not been measured. Indicator LEDs are not enabled. There is no VIA lighting settings panel.

## U.S. and EurKEY profiles

The Mac profile affects only Option+A/O/U handling. With the matching U.S. or EurKEY v1.2 host layout, these produce ä/ö/ü and, with Shift, Ä/Ö/Ü. The physical left Win and Menu keys are Option in the default Mac base layer.

- **U.S. profile:** sends Option+U, releases it, sends the requested letter with optional Shift, then restores held modifiers. Each physical press produces one character; these synthesized chords do not auto-repeat.
- **EurKEY profile:** passes the original chord through for the host layout to handle. Normal host repeat behavior applies.
- Ctrl/Command combinations, including Hyper, pass through unchanged. Windows mode is unaffected.

**Fn+I** toggles the profile in Mac mode. This does not change the computer's input source. There is no automatic per-computer selection or visible profile LED. Release the keys and wait at least three seconds before disconnecting to let the setting save.

If Option+A produces `üa`, the keyboard's U.S. profile is being used with EurKEY on the computer. Switch the keyboard to EurKEY, or select U.S. on the computer. If U.S. produces `å`/`ø`, switch the keyboard to its U.S. profile. Other input layouts and remote/raw-key applications can interpret these chords differently.

## VIA custom keys and saved settings

VIA's **Custom** category provides:

| Index   | Label                              | Action                                      |
| ------- | ---------------------------------- | ------------------------------------------- |
| 0       | WinLk                              | Windows-key lock                            |
| 1–6     | KMod, KHue, KSat, KVal, KSpd, KTog | Lighting controls                           |
| 7       | OS Tog                             | Toggle Windows/Mac; default Fn+O            |
| 8 / 9   | Win OS / Mac OS                    | Explicit persistent OS selection            |
| 10      | US/Eur                             | Toggle Mac input profile; default Mac Fn+I  |
| 11 / 12 | US Kbd / EurKbd                    | Explicit persistent input-profile selection |

Explicit selectors are useful when the current mode is uncertain. Reload the current Design JSON if these names are missing. These are assignable keys, not buttons that immediately change a setting in VIA.

Keymaps, macros, selected OS mode and input profile are saved on the keyboard. VIA layout exports contain key assignments and macros; they do not capture the selected OS mode/profile. After loading an export, choose those settings with the shortcuts or explicit selectors. There are 16 macro slots sharing 1,047 bytes in the five-layer build. Export before flashing; a flash resets the settings store.
