/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once
#include "redragon_ch555.h"

enum k630_layers {
    K630_WINDOWS,
    K630_WINDOWS_FN,
    K630_MAC,
    K630_MAC_FN,
    K630_MAC_FKEYS
};

enum k630_keycodes {
    K630_OS_TOGGLE = RD_KB_SAFE_RANGE,
    K630_OS_WINDOWS,
    K630_OS_MAC,
    K630_LAYOUT_TOGGLE,
    K630_LAYOUT_US,
    K630_LAYOUT_EURKEY
};

bool k630_process_os_keycode(uint16_t keycode, keyrecord_t *record);
bool k630_process_umlaut(uint16_t keycode, keyrecord_t *record);
bool k630_uses_eurkey(void);
