#include "drivers/keyboard.h"

#include <stdbool.h>

#include "cpu/isr.h"
#include "cpu/ports.h"
#include "kernel/logs.h"
#include "kernel/panic.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"

#undef SERVICE
#define SERVICE "DRIVER/KEYBOARD"

static bool     __e0_mode;
static uint32_t __keystate[8];
static char     __keyMap[0xFF]      = {0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '+', '\b', '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static char     __shiftKeyMap[0xFF] = {0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static void    keyboard_callback(registers_t * regs);
static char    keyboard_char(uint8_t code, bool shift);
static bool    get_key_state(uint8_t keycode);
static void    set_key_state(uint8_t keycode, bool state);
static uint8_t get_mods();

void keyboard_init() {
    __e0_mode = false;
    if (!kmemset(__keystate, 0, sizeof(__keystate))) {
        KPANIC("Failed to clear keystate array");
    }
    KLOG_DEBUG("Registering interrupt handler on IRQ 1");
    register_interrupt_handler(IRQ1, keyboard_callback);
    KLOG_DEBUG("Initialized driver");
}

static bool get_key_state(uint8_t keycode) {
    int i = keycode / 32;
    int b = keycode % 32;

    return __keystate[i] & (1 << b);
}

static void set_key_state(uint8_t keycode, bool state) {
    int i = keycode / 32;
    int b = keycode % 32;

    if (state) {
        __keystate[i] |= (1 << b);
    }
    else {
        __keystate[i] &= ~(1 << b);
    }
}

static uint8_t get_mods() {
    uint8_t mods = 0;
    // TODO handle 0xE0 to get right ctrl
    if (get_key_state(KEY_LCTRL)) {
        mods |= KEY_MOD_CTRL;
    }
    // TODO handle 0xE0 to get right alt
    if (get_key_state(KEY_LALT)) {
        mods |= KEY_MOD_ALT;
    }
    if (get_key_state(KEY_LSHIFT) || get_key_state(KEY_RSHIFT)) {
        mods |= KEY_MOD_SHIFT;
    }
    // TODO handle 0xE0 to get right super
    if (get_key_state(KEY_SUPER)) {
        mods |= KEY_MOD_SUPER;
    }
    return mods;
}

static char keyboard_char(uint8_t code, bool shift) {
    code = code & 0x7F;
    if (shift) {
        return __shiftKeyMap[code];
    }
    return __keyMap[code];
}

static void keyboard_callback(registers_t * regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    uint8_t scancode = port_byte_in(0x60);
    if (scancode == 0xE0) {
        __e0_mode = true;
        return;
    }
    // printf("%02X ", scancode);
    uint8_t          keycode   = scancode;
    keyboard_event_t key_event = KEY_EVENT_PRESS;
    uint8_t          mods      = get_mods();
    bool             press     = keycode < 0x80;

    if (press) {
        if (get_key_state(keycode)) {
            key_event = KEY_EVENT_REPEAT;
        }
        set_key_state(keycode, true);
    }

    else {
        keycode -= 0x80;
        key_event = KEY_EVENT_RELEASE;
        set_key_state(keycode, false);
    }

    char c = keyboard_char(keycode, mods & KEY_MOD_SHIFT);

    KLOG_TRACE("Keyboard interrupt callback scancode=0x%02X keycode=0x%02X c=%c press=%u", scancode, keycode, c, press);

    ebus_event_t event;
    event.event_id     = EBUS_EVENT_KEY;
    event.key.event    = key_event;
    event.key.mods     = mods;
    event.key.c        = c;
    event.key.keycode  = keycode;
    event.key.scancode = scancode;
    queue_event(&event);
    __e0_mode = false;
}
