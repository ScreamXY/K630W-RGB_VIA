/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "k630_vs11k28a.h"
#ifdef VIA_ENABLE
#    include "via.h"
#endif

static uint8_t os_base_layer(void) {
    return get_highest_layer(default_layer_state) == K630_MAC ? K630_MAC : K630_WINDOWS;
}

bool k630_process_os_keycode(uint16_t keycode, keyrecord_t *record) {
    uint8_t target;
    switch (keycode) {
        case K630_OS_TOGGLE:
            target = os_base_layer() == K630_MAC ? K630_WINDOWS : K630_MAC;
            break;
        case K630_OS_WINDOWS:
            target = K630_WINDOWS;
            break;
        case K630_OS_MAC:
            target = K630_MAC;
            break;
        default:
            return true;
    }
    if (record->event.pressed && target != os_base_layer()) {
        // A held Fn or modifier must not leave an old layer/key active when
        // its mapping changes. QMK's source-layer cache handles their releases.
        clear_keyboard();
        layer_clear();
        set_single_persistent_default_layer(target);
    }
    return false;
}

bool k630_win_lock_exempt(uint16_t keycode) {
    // Win Lock (Windows Fn+Win) blocks GUI keycodes, which are Command in Mac
    // mode. A lock left on before Fn+O must not disable Mac shortcuts.
    return (keycode == KC_LGUI || keycode == KC_RGUI) && os_base_layer() == K630_MAC;
}

#ifdef VIA_ENABLE
void via_custom_value_command_kb(uint8_t *data, uint8_t length) {
    // Read-only diagnostics on channel 'K': 1 = OS, 2 = Mac input profile.
    if (length >= 4 && data[0] == id_custom_get_value && data[1] == 0x4b &&
        (data[2] == 1 || data[2] == 2)) {
        data[3] = data[2] == 1 ? os_base_layer() : k630_uses_eurkey();
    } else if (length) {
        data[0] = id_unhandled;
    }
}
#endif
