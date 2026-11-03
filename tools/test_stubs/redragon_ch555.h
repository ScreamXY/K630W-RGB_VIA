// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef uint32_t layer_state_t;
typedef struct { struct { bool pressed; struct { uint8_t row, col; } key; } event; } keyrecord_t;
#define MATRIX_ROWS 6
#define MATRIX_COLS 16
#define MOD_MASK_CTRL 0x11
#define MOD_MASK_SHIFT 0x22
#define MOD_MASK_ALT 0x44
#define MOD_MASK_GUI 0x88
#define MOD_BIT(kc) (1U << ((kc) & 7))
enum { KC_A = 4, KC_B = 5, KC_O = 18, KC_U = 24, KC_LALT = 0xe2, KC_LGUI = 0xe3, KC_RGUI = 0xe7 };
enum { RD_KB_SAFE_RANGE = 0x7e07 };
extern layer_state_t default_layer_state;
uint8_t get_highest_layer(layer_state_t state);
void clear_keyboard(void);
void layer_clear(void);
void set_single_persistent_default_layer(uint8_t layer);
uint8_t get_mods(void);
uint8_t get_weak_mods(void);
void set_mods(uint8_t mods);
void set_weak_mods(uint8_t mods);
void clear_weak_mods(void);
bool is_key_pressed(uint8_t key);
void send_keyboard_report(void);
void tap_code_delay(uint8_t key, uint16_t delay);
uint32_t eeconfig_read_kb(void);
void eeconfig_update_kb(uint32_t value);
