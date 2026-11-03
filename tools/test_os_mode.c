// SPDX-License-Identifier: GPL-2.0-or-later
// Exercise the firmware's actual OS-switch handler with QMK state boundaries.
#include <assert.h>
#include <stdio.h>
#include "k630_vs11k28a.h"
#include "via.h"

layer_state_t default_layer_state = 1;
static layer_state_t active_layers, saved_layer = 1;
static unsigned saves, held_modifiers, held_keys;
static bool eurkey;
bool k630_uses_eurkey(void) { return eurkey; }

uint8_t get_highest_layer(layer_state_t state) {
    uint8_t highest = 0;
    while (state >>= 1) {
        highest++;
    }
    return highest;
}
void clear_keyboard(void) { held_modifiers = held_keys = 0; }
void layer_clear(void) { active_layers = 0; }
void set_single_persistent_default_layer(uint8_t layer) {
    assert(!held_modifiers && !held_keys && !active_layers);
    default_layer_state = saved_layer = (layer_state_t)1 << layer;
    saves++;
}
static bool event(uint16_t code, bool pressed) {
    keyrecord_t record = {.event = {.pressed = pressed}};
    return k630_process_os_keycode(code, &record);
}
int main(void) {
    assert(event(4, true)); // A and ordinary keys pass through untouched.
    // Win Lock applies in Windows mode only; Mac Command must never be locked.
    assert(!k630_win_lock_exempt(KC_LGUI) && !k630_win_lock_exempt(KC_RGUI));
    assert(!event(K630_OS_WINDOWS, true) && saves == 0);
    active_layers = 1 << K630_WINDOWS_FN;
    held_modifiers = 0x08; held_keys = 1;
    assert(!event(K630_OS_TOGGLE, true));
    assert(default_layer_state == 4 && saved_layer == 4 && saves == 1);
    assert(!active_layers && !held_modifiers && !held_keys);
    assert(k630_win_lock_exempt(KC_LGUI) && k630_win_lock_exempt(KC_RGUI));
    assert(!k630_win_lock_exempt(KC_LALT) && !k630_win_lock_exempt(KC_A));
    assert(!event(K630_OS_TOGGLE, false) && saves == 1); // Release must not toggle back.
    default_layer_state = saved_layer; // Reboot reloads QMK's persisted default.
    uint8_t status[32] = {8, 0x4b, 1};
    via_custom_value_command_kb(status, sizeof(status));
    assert(status[0] == 8 && status[3] == K630_MAC);
    assert(!event(K630_OS_MAC, true) && saves == 1); // No redundant EEPROM write.
    active_layers = 1 << K630_MAC_FN;
    assert(!event(K630_OS_TOGGLE, true));
    assert(default_layer_state == 1 && saved_layer == 1 && saves == 2 && !active_layers);
    assert(!event(K630_OS_TOGGLE, false) && saves == 2);
    assert(!event(K630_OS_MAC, true) && saved_layer == 4 && saves == 3);
    assert(!event(K630_OS_WINDOWS, true) && saved_layer == 1 && saves == 4);
    // Diagnostics may only read. SET/SAVE and malformed requests are refused.
    for (unsigned cmd = 0; cmd < 256; cmd++) {
        uint8_t request[32] = {(uint8_t)cmd, 0x4b, 1};
        via_custom_value_command_kb(request, sizeof(request));
        assert(request[0] == (cmd == 8 ? 8 : 255));
        assert(saved_layer == 1 && saves == 4);
    }
    uint8_t short_request[3] = {8, 0x4b, 1};
    via_custom_value_command_kb(short_request, sizeof(short_request));
    assert(short_request[0] == 255);
    for (unsigned profile = 0; profile < 2; profile++) {
        eurkey = profile;
        for (unsigned cmd = 0; cmd < 256; cmd++) {
            uint8_t request[32] = {(uint8_t)cmd, 0x4b, 2};
            via_custom_value_command_kb(request, sizeof(request));
            assert(request[0] == (cmd == 8 ? 8 : 255));
            if (cmd == 8) {
                assert(request[3] == profile);
            }
            assert(eurkey == profile && saved_layer == 1 && saves == 4);
        }
    }
    uint8_t unknown[32] = {8, 0x4b, 3};
    via_custom_value_command_kb(unknown, sizeof(unknown));
    assert(unknown[0] == 255);
    puts("OS switching: both directions, saved state, held keys/Fn cleanup, release, Win Lock scope, and read-only diagnostics passed.");
}
