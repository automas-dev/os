#ifndef DRIVER_PIT_H
#define DRIVER_PIT_H

#include <stdint.h>

// bits 5 and 4
enum PIT_ACCESS_MODE {
    PIT_ACCESS_MODE_LATCH    = 0x00,
    PIT_ACCESS_MODE_LOW      = 0x10,
    PIT_ACCESS_MODE_HIGH     = 0x20,
    PIT_ACCESS_MODE_LOW_HIGH = 0x30,
};

// bits 3 - 1
enum PIT_CHANNEL_MODE {
    PIT_CHANNEL_MODE_0_INT_TERM_COUNT      = 0x00, // interrupt on terminal count
    PIT_CHANNEL_MODE_1_HARDWARE_ONESHOT    = 0x02, // hardware re-triggerable one-shot
    PIT_CHANNEL_MODE_2_RATE_GEN            = 0x04, // rate generator
    PIT_CHANNEL_MODE_3_SQUARE_WAVE_GEN     = 0x06, // square wave generator
    PIT_CHANNEL_MODE_4_SW_STROBE           = 0x08, // software triggered strobe
    PIT_CHANNEL_MODE_5_HW_STROBE           = 0x0a, // hardware triggered strobe
    PIT_CHANNEL_MODE_2_RATE_GEN_ALT        = 0x0c, // same as mode 2
    PIT_CHANNEL_MODE_3_SQUARE_WAVE_GEN_ALT = 0x0e, // same as mode 3
};

// bit 0 is bcd / binary mode, should always be 0

void pit_init();

int pit_write_channel(uint8_t channel, uint8_t access_mode, uint8_t channel_mode, uint16_t reload_value);

// uint8_t pit_read_channel(uint8_t channel);

// int pit_read_count(uint8_t channel);

#endif // DRIVER_PIT_H
