// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the actual handler, including its emitted modifier/key sequence.
#include <assert.h>
#include <stdio.h>
#include "k630_vs11k28a.h"

layer_state_t default_layer_state;
static uint8_t mods, weak, held_key;
static unsigned taps, reports;
static uint8_t tapped[2], tap_mods[2];
static uint32_t stored_config;
static unsigned config_writes;
uint32_t eeconfig_read_kb(void) { return stored_config; }
void eeconfig_update_kb(uint32_t value) { stored_config = value; config_writes++; }
uint8_t get_highest_layer(layer_state_t state) {
    uint8_t highest = 0;
    while (state >>= 1) {
        highest++;
    }
    return highest;
}
uint8_t get_mods(void) { return mods; }
uint8_t get_weak_mods(void) { return weak; }
void set_mods(uint8_t value) { mods = value; }
void set_weak_mods(uint8_t value) { weak = value; }
void clear_weak_mods(void) { weak = 0; }
bool is_key_pressed(uint8_t key) { return key == held_key; }
void send_keyboard_report(void) { reports++; }
void tap_code_delay(uint8_t key, uint16_t delay) {
    assert(taps < 2 && delay >= 5);
    tapped[taps] = key;
    tap_mods[taps++] = mods | weak;
}
static bool event(uint16_t code, bool pressed, uint8_t row, uint8_t col) {
    keyrecord_t record = {.event = {.pressed = pressed, .key = {row, col}}};
    return k630_process_umlaut(code, &record);
}
int main(void) {
    // Every modifier combination, in both OS modes; Hyper must pass untouched.
    const uint8_t letters[] = {KC_A, KC_O, KC_U};
    for (unsigned profile = 0; profile < 2; profile++) {
        stored_config = profile;
        assert(k630_uses_eurkey() == (bool)profile);
        for (unsigned os = 0; os <= 2; os += 2) {
            for (unsigned i = 0; i < sizeof(letters); i++) {
                for (unsigned m = 0; m < 256; m++) {
                    default_layer_state = 1U << os;
                    mods = m; weak = 0; taps = reports = 0;
                    bool intercept = !profile && os == K630_MAC && (m & MOD_MASK_ALT) &&
                                     !(m & (MOD_MASK_CTRL | MOD_MASK_GUI));
                    assert(event(letters[i], true, 2, 15) == !intercept);
                    assert(mods == m && weak == 0);
                    if (intercept) {
                        assert(taps == 2 && reports == 3);
                        assert(tapped[0] == KC_U && tap_mods[0] == 4);
                        assert(tapped[1] == letters[i] && tap_mods[1] == (m & MOD_MASK_SHIFT));
                    } else {
                        assert(taps == 0 && reports == 0);
                    }
                    // Option release and an OS/keymap change before letter release.
                    mods = 0; default_layer_state = 1;
                    assert(event(KC_B, false, 2, 15) == !intercept);
                    assert(event(KC_B, false, 2, 15));
                }
            }
        }
    }
    stored_config = 0;
    default_layer_state = 1 << K630_MAC;
    // Weak modifiers are included in filtering and restored separately.
    mods = 0x40; weak = 0x20; taps = reports = 0;
    assert(!event(KC_A, true, 3, 1));
    assert(mods == 0x40 && weak == 0x20 && tap_mods[1] == 0x20);
    assert(!event(KC_A, false, 3, 1));
    weak = 1; taps = reports = 0;
    assert(event(KC_A, true, 3, 1) && taps == 0 && reports == 0);
    weak = 0;
    // Do not synthesize releases of already-held keys; unrelated keys pass.
    for (unsigned i = 0; i < sizeof(letters); i++) {
        held_key = letters[i];
        assert(event(letters[i], true, 3, 1));
    }
    held_key = KC_U;
    assert(event(KC_A, true, 3, 1));
    held_key = 0;
    assert(event(KC_B, true, 3, 1));
    assert(event(KC_A, true, MATRIX_ROWS, 0));
    assert(event(KC_A, true, 0, MATRIX_COLS));
    // Two held umlaut keys retain independent release tracking.
    taps = 0; assert(!event(KC_A, true, 3, 1));
    taps = 0; assert(!event(KC_O, true, 2, 9));
    assert(event(KC_B, false, 0, 0));
    assert(!event(KC_A, false, 3, 1));
    assert(!event(KC_O, false, 2, 9));

    // Toggle/explicit selectors persist without touching the OS, held
    // modifiers or other reserved config bits; key-up never toggles twice.
    stored_config = 0xa5a50000; config_writes = taps = reports = 0;
    mods = 0x40; weak = 0x20;
    assert(!event(K630_LAYOUT_US, true, 2, 8) && config_writes == 0);
    assert(!event(K630_LAYOUT_TOGGLE, true, 2, 8));
    assert(k630_uses_eurkey() && stored_config == 0xa5a50001 && config_writes == 1);
    assert(!event(K630_LAYOUT_TOGGLE, false, 2, 8) && config_writes == 1);
    assert(!event(K630_LAYOUT_EURKEY, true, 2, 8) && config_writes == 1);
    assert(!event(K630_LAYOUT_US, true, 2, 8));
    assert(!k630_uses_eurkey() && stored_config == 0xa5a50000 && config_writes == 2);
    assert(!event(K630_LAYOUT_EURKEY, true, 2, 8) && config_writes == 3);
    assert(!event(K630_LAYOUT_TOGGLE, true, 2, 8));
    assert(!k630_uses_eurkey() && config_writes == 4);
    assert(default_layer_state == (1 << K630_MAC) && mods == 0x40 && weak == 0x20);
    assert(!taps && !reports);
    // A press consumed in U.S. mode remains consumed after profile switching.
    assert(!event(KC_A, true, 3, 1));
    assert(!event(K630_LAYOUT_EURKEY, true, 2, 8));
    assert(!event(KC_A, false, 3, 1));
    // A press passed through in EurKEY still gets its normal release in U.S.
    assert(event(KC_A, true, 3, 1));
    assert(!event(K630_LAYOUT_US, true, 2, 8));
    assert(event(KC_A, false, 3, 1));
    puts("Umlauts: all modifiers and OS/input profiles, exact sequences, releases and persistent selectors passed.");
}
