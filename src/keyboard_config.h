/*
 * Raise2 configuration file
 * This file is used to define the hardware configuration of the Raise2 keyboard.
 * It is used by the Kaleidoscope firmware to configure the keyboard.
 * The configuration is used to define the number of keys, the number of LEDs, and the layout of the keyboard.
 */
#pragma once
#include <cstdint>

#define KEYBOARD_NEURON_FW_VERSION RAISE2_NEURON_FW_VERSION

#define SHORT_NAME "raise2"

// LED definitions
#define KEY_MATRIX_LEDS 33
#define UNDERGLOW_LEDS_LEFT_SIDE 53
#define LEDS_HAND_LEFT 33
#define UNDERGLOW_LEDS_RIGHT_SIDE 54
#define LEDS_HAND_RIGHT 36
#define NEURON_LED 0

#define LED_MAP {                                                                                                               \
    /*left side - 33 keys includes LP: key 19 is missing for ANSI layout*/                                                      \
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,   \
                                                                                                                                \
    /* right side - 36 keys includes LP*/                                                                                       \
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, \
    64, 65, 66, 67, 68,                                                                                                         \
                                                                                                                                \
    /*left under glow - 53*/                                                                                                    \
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93,                         \
    94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115,                     \
    116, 117, 118, 119, 120, 121,                                                                                               \
                                                                                                                                \
    /* right underglow - 54*/                                                                                                   \
    122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145,     \
    146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169,     \
    170, 171, 172, 173, 174, 175}

// For retrocompatibility with the old keymap. It is not used anymore. You can leave it as it is.
#define KEY_LED_MAP {                                                                                                                                                                                     \
    0, 1, 2, 3, 4, 5, 6, no_led, no_led, 6 + key_matrix_leds, 5 + key_matrix_leds, 4 + key_matrix_leds, 3 + key_matrix_leds, 2 + key_matrix_leds, 1 + key_matrix_leds, 0 + key_matrix_leds,               \
    7, 8, 9, 10, 11, 12, 13, no_led, no_led, 13 + key_matrix_leds, 12 + key_matrix_leds, 11 + key_matrix_leds, 10 + key_matrix_leds, 9 + key_matrix_leds, 8 + key_matrix_leds, 7 + key_matrix_leds,       \
    14, 15, 16, 17, 18, 19, 20, no_led, no_led, 20 + key_matrix_leds, 19 + key_matrix_leds, 18 + key_matrix_leds, 17 + key_matrix_leds, 16 + key_matrix_leds, 15 + key_matrix_leds, 14 + key_matrix_leds, \
    21, 22, 23, 24, 25, 26, no_led, no_led, no_led, no_led, 26 + key_matrix_leds, 25 + key_matrix_leds, 24 + key_matrix_leds, 23 + key_matrix_leds, 22 + key_matrix_leds,                                 \
    21 + key_matrix_leds,                                                                                                                                                                                 \
    27, 28, 29, 30, 31, 32, 33, 34, 34 + key_matrix_leds, 33 + key_matrix_leds, 32 + key_matrix_leds, 31 + key_matrix_leds, 30 + key_matrix_leds, 29 + key_matrix_leds, 28 + key_matrix_leds, 27 + key_matrix_leds}

// Matrix definitions
#define MATRIX_ROWS 5
#define MATRIX_COLS 16
#define LEFT_COLUMNS 8

#define PER_KEY_DATA(dflt,                                                                                  \
  r0c0, r0c1, r0c2, r0c3, r0c4, r0c5, r0c6,                r0c9,  r0c10, r0c11, r0c12, r0c13, r0c14, r0c15, \
  r1c0, r1c1, r1c2, r1c3, r1c4, r1c5,               r1c8,  r1c9,  r1c10, r1c11, r1c12, r1c13, r1c14, r1c15, \
  r2c0, r2c1, r2c2, r2c3, r2c4, r2c5,                      r2c9,  r2c10, r2c11, r2c12, r2c13, r2c14, r2c15, \
  r3c0, r3c1, r3c2, r3c3, r3c4, r3c5, r3c6,                       r3c10, r3c11, r3c12, r3c13, r3c14, r3c15, \
  r4c0, r4c1, r4c2, r4c3, r4c4,                                   r4c10, r4c11, r4c12, r4c13, r4c14, r4c15, \
                          r4c5, r4c6,                             r4c8,  r4c9                               \
  )                                                                                                         \
    r0c0, r0c1, r0c2, r0c3, r0c4, r0c5, r0c6, dflt, dflt, r0c9, r0c10, r0c11, r0c12, r0c13, r0c14, r0c15,   \
    r1c0, r1c1, r1c2, r1c3, r1c4, r1c5, dflt, dflt, r1c8, r1c9, r1c10, r1c11, r1c12, r1c13, r1c14, r1c15,   \
    r2c0, r2c1, r2c2, r2c3, r2c4, r2c5, dflt, dflt, dflt, r2c9, r2c10, r2c11, r2c12, r2c13, r2c14, r2c15,   \
    r3c0, r3c1, r3c2, r3c3, r3c4, r3c5, r3c6, dflt, dflt, dflt, r3c10, r3c11, r3c12, r3c13, r3c14, r3c15,   \
    r4c0, r4c1, r4c2, r4c3, r4c4, r4c5, r4c6, dflt, r4c8, r4c9, r4c10, r4c11, r4c12, r4c13, r4c14, r4c15

#define PER_KEY_DATA_STACKED(dflt,                          \
  r0c0, r0c1, r0c2, r0c3, r0c4, r0c5, r0c6,                 \
  r1c0, r1c1, r1c2, r1c3, r1c4, r1c5,                       \
  r2c0, r2c1, r2c2, r2c3, r2c4, r2c5,                       \
  r3c0, r3c1, r3c2, r3c3, r3c4, r3c5, r3c6,                 \
  r4c0, r4c1, r4c2, r4c3, r4c4,                             \
                          r4c5, r4c6,                       \
                                                            \
         r0c9,  r0c10, r0c11, r0c12, r0c13, r0c14, r0c15,   \
  r1c8,  r1c9,  r1c10, r1c11, r1c12, r1c13, r1c14, r1c15,   \
         r2c9,  r2c10, r2c11, r2c12, r2c13, r2c14, r2c15,   \
                r3c10, r3c11, r3c12, r3c13, r3c14, r3c15,   \
                r4c10, r4c11, r4c12, r4c13, r4c14, r4c15,   \
                r4c8,  r4c9                                 \
)                                                           \
    r0c0, r0c1, r0c2, r0c3, r0c4, r0c5, r0c6, dflt, dflt, r0c9, r0c10, r0c11, r0c12, r0c13, r0c14, r0c15,   \
    r1c0, r1c1, r1c2, r1c3, r1c4, r1c5, dflt, dflt, r1c8, r1c9, r1c10, r1c11, r1c12, r1c13, r1c14, r1c15,   \
    r2c0, r2c1, r2c2, r2c3, r2c4, r2c5, dflt, dflt, dflt, r2c9, r2c10, r2c11, r2c12, r2c13, r2c14, r2c15,   \
    r3c0, r3c1, r3c2, r3c3, r3c4, r3c5, r3c6, dflt, dflt, dflt, r3c10, r3c11, r3c12, r3c13, r3c14, r3c15,   \
    r4c0, r4c1, r4c2, r4c3, r4c4, r4c5, r4c6, dflt, r4c8, r4c9, r4c10, r4c11, r4c12, r4c13, r4c14, r4c15