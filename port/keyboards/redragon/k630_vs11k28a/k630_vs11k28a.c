/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "k630_vs11k28a.h"
#include "led_dma.h"

void keyboard_post_init_kb(void) {
    uint16_t slot;
    /* Conservative initial intensity for the 200 mA-labelled K630.
     * Current draw still needs a hardware measurement. */
    for (slot = 0; slot < LED_DMA_COMS * LED_DMA_GROUPS; slot++) {
        led_dma_buf[slot * 4] = 0x80;
    }
    redragon_init();
    keyboard_post_init_user();
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    return process_record_user(keycode, record) && k630_process_os_keycode(keycode, record) &&
           k630_process_umlaut(keycode, record) &&
           (k630_win_lock_exempt(keycode) || redragon_process_record(keycode, record));
}

void housekeeping_task_kb(void) {
    redragon_task();
}

void suspend_power_down_kb(void) {
    redragon_suspend(true);
    suspend_power_down_user();
}

void suspend_wakeup_init_kb(void) {
    redragon_suspend(false);
    suspend_wakeup_init_user();
}
