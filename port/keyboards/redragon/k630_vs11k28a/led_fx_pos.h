/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

/* Populated K630 slots only; unused slots and unverified indicators stay dark. */
static __code const uint8_t led_pos[LED_FX_COLUMNS * LED_FX_ROWS] = {
    LED_FX_SKIP, 6, 10, 11, 14, 8, LED_FX_SKIP, LED_FX_SKIP, // COM 0
    LED_FX_SKIP, 19, 25, 29, LED_FX_SKIP, 24, LED_FX_SKIP, LED_FX_SKIP, // COM 1
    LED_FX_SKIP, 32, 38, 41, 35, 40, LED_FX_SKIP, LED_FX_SKIP, // COM 2
    LED_FX_SKIP, 45, 51, 54, 48, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 3
    LED_FX_SKIP, 57, 64, 67, 60, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 4
    LED_FX_SKIP, 70, 76, 80, 73, 88, LED_FX_SKIP, LED_FX_SKIP, // COM 5
    LED_FX_SKIP, 83, 89, 92, 86, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 6
    LED_FX_SKIP, 96, 102, 105, 99, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 7
    LED_FX_SKIP, 108, 115, 118, 111, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 8
    LED_FX_SKIP, 121, 127, 131, 124, 135, LED_FX_SKIP, LED_FX_SKIP, // COM 9
    LED_FX_SKIP, 134, 140, 143, 137, 151, LED_FX_SKIP, LED_FX_SKIP, // COM 10
    LED_FX_SKIP, 146, 153, 156, 150, 167, LED_FX_SKIP, LED_FX_SKIP, // COM 11
    LED_FX_SKIP, 159, 166, LED_FX_SKIP, LED_FX_SKIP, 183, LED_FX_SKIP, LED_FX_SKIP, // COM 12
    LED_FX_SKIP, 178, 181, 177, 173, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 13
    LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 14
    LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, LED_FX_SKIP, // COM 15
};

static __code const uint8_t led_row[LED_FX_COLUMNS * LED_FX_ROWS] = {
    0, 0, 1, 2, 3, 4, 0, 0, // COM 0
    0, 0, 1, 2, 0, 4, 0, 0, // COM 1
    0, 0, 1, 2, 3, 4, 0, 0, // COM 2
    0, 0, 1, 2, 3, 0, 0, 0, // COM 3
    0, 0, 1, 2, 3, 0, 0, 0, // COM 4
    0, 0, 1, 2, 3, 4, 0, 0, // COM 5
    0, 0, 1, 2, 3, 0, 0, 0, // COM 6
    0, 0, 1, 2, 3, 0, 0, 0, // COM 7
    0, 0, 1, 2, 3, 0, 0, 0, // COM 8
    0, 0, 1, 2, 3, 4, 0, 0, // COM 9
    0, 0, 1, 2, 3, 4, 0, 0, // COM 10
    0, 0, 1, 2, 3, 4, 0, 0, // COM 11
    0, 0, 1, 0, 0, 4, 0, 0, // COM 12
    0, 0, 1, 2, 3, 0, 0, 0, // COM 13
    0, 0, 0, 0, 0, 0, 0, 0, // COM 14
    0, 0, 0, 0, 0, 0, 0, 0, // COM 15
};
