/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "k630_vs11k28a.h"

/* Remember physical presses so releasing Option, changing layers, or switching
 * OS before the letter is released cannot turn its release into a normal key. */
static uint16_t consumed[MATRIX_ROWS];

/* QMK initializes the reserved keyboard-config word to zero: U.S. mode.
 * Keep this separate from the selected OS and from VIA's editable layers. */
#define EURKEY_CONFIG_BIT 1UL

bool k630_uses_eurkey(void) {
    return (eeconfig_read_kb() & EURKEY_CONFIG_BIT) != 0;
}

static bool process_layout_keycode(uint16_t keycode, keyrecord_t *record) {
    uint32_t config = eeconfig_read_kb();
    uint32_t target;
    switch (keycode) {
        case K630_LAYOUT_TOGGLE:
            target = config ^ EURKEY_CONFIG_BIT;
            break;
        case K630_LAYOUT_US:
            target = config & ~EURKEY_CONFIG_BIT;
            break;
        case K630_LAYOUT_EURKEY:
            target = config | EURKEY_CONFIG_BIT;
            break;
        default:
            return true;
    }
    if (record->event.pressed && target != config) {
        eeconfig_update_kb(target);
    }
    return false;
}

bool k630_process_umlaut(uint16_t keycode, keyrecord_t *record) {
    if (keycode >= K630_LAYOUT_TOGGLE && keycode <= K630_LAYOUT_EURKEY) {
        return process_layout_keycode(keycode, record);
    }
    uint8_t row = record->event.key.row;
    uint8_t col = record->event.key.col;
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return true;
    }
    uint16_t bit = (uint16_t)1 << col;
    if (!record->event.pressed) {
        if (!(consumed[row] & bit)) {
            return true;
        }
        consumed[row] &= ~bit;
        return false;
    }

    uint8_t mods = get_mods();
    uint8_t weak = get_weak_mods();
    uint8_t combined = mods | weak;
    if (get_highest_layer(default_layer_state) != K630_MAC ||
        !(combined & MOD_MASK_ALT) || (combined & (MOD_MASK_CTRL | MOD_MASK_GUI)) ||
        (keycode != KC_A && keycode != KC_O && keycode != KC_U) ||
        k630_uses_eurkey()) {
        return true;
    }

    /* Never release a U or target letter that another physical key is holding. */
    if (is_key_pressed(KC_U) || is_key_pressed((uint8_t)keycode)) {
        return true;
    }
    consumed[row] |= bit;

    /* macOS U.S./ABC: Option+U dead key, then the letter with its original Shift.
     * Temporarily remove both left/right Option and weak modifiers, then restore
     * their exact states. Ctrl/Command/Hyper chords are excluded above. */
    clear_weak_mods();
    set_mods(MOD_BIT(KC_LALT));
    send_keyboard_report();
    tap_code_delay(KC_U, 5);
    set_mods(combined & MOD_MASK_SHIFT);
    send_keyboard_report();
    tap_code_delay((uint8_t)keycode, 5);
    set_mods(mods);
    set_weak_mods(weak);
    send_keyboard_report();
    return false;
}
