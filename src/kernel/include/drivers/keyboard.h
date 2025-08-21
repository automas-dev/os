#ifndef DRIVER_KEYBOARD_H
#define DRIVER_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>

typedef enum keyboard_event {
    KEY_EVENT_PRESS = 0,
    KEY_EVENT_RELEASE,
    KEY_EVENT_REPEAT,
} keyboard_event_t;

typedef enum keyboard_mod {
    KEY_MOD_CTRL  = 0x1,
    KEY_MOD_ALT   = 0x2,
    KEY_MOD_SHIFT = 0x4,
    KEY_MOD_SUPER = 0x8,
} keyboard_mod_t;

typedef enum keyboard_key {
    KEY_NULL      = 0,
    KEY_ESC       = 0x1,
    KEY_1         = 0x2,
    KEY_2         = 0x3,
    KEY_3         = 0x4,
    KEY_4         = 0x5,
    KEY_5         = 0x6,
    KEY_6         = 0x7,
    KEY_7         = 0x8,
    KEY_8         = 0x9,
    KEY_9         = 0x0A,
    KEY_0         = 0x0B,
    KEY_DASH      = 0x0C,
    KEY_PLUS      = 0x0D,
    KEY_BACKSPACE = 0x0E,
    KEY_TAB       = 0x0F,
    KEY_Q         = 0x10,
    KEY_W         = 0x11,
    KEY_E         = 0x12,
    KEY_R         = 0x13,
    KEY_T         = 0x14,
    KEY_Y         = 0x15,
    KEY_U         = 0x16,
    KEY_I         = 0x17,
    KEY_O         = 0x18,
    KEY_P         = 0x19,
    KEY_LSQR      = 0x1A,
    KEY_RSQR      = 0x1B,
    KEY_ENTER     = 0x1C,
    KEY_LCTRL     = 0x1D,
    KEY_A         = 0x1E,
    KEY_S         = 0x1F,
    KEY_D         = 0x20,
    KEY_F         = 0x21,
    KEY_G         = 0x22,
    KEY_H         = 0x23,
    KEY_J         = 0x24,
    KEY_K         = 0x25,
    KEY_L         = 0x26,
    KEY_SEMI      = 0x27,
    KEY_QUOTE     = 0x28,
    KEY_GRAVE     = 0x29,
    KEY_LSHIFT    = 0x2A,
    KEY_BACKSLASH = 0x2B,
    KEY_Z         = 0x2C,
    KEY_X         = 0x2D,
    KEY_C         = 0x2E,
    KEY_V         = 0x2F,
    KEY_B         = 0x30,
    KEY_N         = 0x31,
    KEY_M         = 0x32,
    KEY_COMMA     = 0x33,
    KEY_PERIOD    = 0x34,
    KEY_SLASH     = 0x35,
    KEY_RSHIFT    = 0x36,
    KEY_NUM_STAR  = 0x37,
    KEY_LALT      = 0x38,
    KEY_SPACE     = 0x39,
    END_OF_KEYS,
    KEY_SUPER = 0x5B,
} keyboard_key_t;

void init_keyboard();

#endif // DRIVER_KEYBOARD_H
