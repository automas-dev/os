#include "drivers/pit.h"

#include "cpu/isr.h"
#include "cpu/ports.h"
#include "kernel/logs.h"
#include "kernel/panic.h"
#include "libc/string.h"

#undef SERVICE
#define SERVICE "DRIVER/PIT"

#define PIT_CHANNEL_0_PORT 0x40
#define PIT_CHANNEL_1_PORT 0x41
#define PIT_CHANNEL_2_PORT 0x42
#define PIT_CONTROL_PORT   0x43

// bits 7 and 6
enum PIT_CHANNEL {
    PIT_CHANNEL_0 = 0x00,
    PIT_CHANNEL_1 = 0x40,
    PIT_CHANNEL_2 = 0x80,
    // PIT_CHANNEL_READ_BACK = 0xc0,
};

typedef struct _channel {
    uint8_t  channel;
    uint8_t  access_mode;
    uint8_t  mode;
    uint16_t reload_value;
} pit_channel_t;

static pit_channel_t __channels[3];

void pit_init() {
    KLOG_DEBUG("Initialize PIT driver");
    if (!kmemset(__channels, 0, sizeof(__channels))) {
        KPANIC("Failed to clear channels array");
    }

    __channels[0].channel = PIT_CHANNEL_0;
    __channels[1].channel = PIT_CHANNEL_1;
    __channels[2].channel = PIT_CHANNEL_2;
}

int pit_write_channel(uint8_t channel, uint8_t access_mode, uint8_t channel_mode, uint16_t reload_value) {
    KLOG_TRACE("Write channel %u access_mode=0x%02X channel_mode=0x%02X reload_value=0x%04X", channel, access_mode, channel_mode, reload_value);
    if (channel > 2) {
        KLOG_ERROR("Attempt write to invalid channel %u, must be < 3", channel);
        return -1;
    }

    __channels[channel].access_mode  = access_mode;
    __channels[channel].mode         = channel_mode;
    __channels[channel].reload_value = reload_value;

    uint8_t cmd = __channels[channel].channel | __channels[channel].access_mode | channel_mode;

    disable_interrupts();

    port_byte_out(PIT_CONTROL_PORT, cmd);

    port_byte_out(PIT_CHANNEL_0_PORT + channel, reload_value & 0xff);
    port_byte_out(PIT_CHANNEL_0_PORT + channel, (reload_value >> 8) & 0xff);

    enable_interrupts();

    return 0;
}

// uint8_t pit_read_channel(uint8_t channel) {
//     KLOG_TRACE("Read channel %u", channel);
//     if (channel > 2) {
//         KLOG_ERROR("Attempt read from invalid channel %u, must be < 3", channel);
//         return 0;
//     }

//     uint8_t cmd = PIT_CHANNEL_READ_BACK | (1 << (channel + 1));

//     disable_interrupts();

//     port_byte_out(PIT_CONTROL_PORT, cmd);

//     uint8_t status = port_byte_in(PIT_CONTROL_PORT);

//     enable_interrupts();

//     return status;
// }

// int pit_read_count(uint8_t channel) {
//     KLOG_TRACE("Read count channel %u", channel);
//     if (channel > 2) {
//         KLOG_ERROR("Attempt read count from invalid channel %u, must be < 3", channel);
//         return -1;
//     }

//     disable_interrupts();

//     port_byte_out(PIT_CHANNEL_0_PORT + channel, 0);

//     int count = port_byte_in(PIT_CHANNEL_0_PORT + channel);

//     if (__channels[channel].access_mode == PIT_ACCESS_MODE_LOW_HIGH) {
//         count |= port_byte_in(PIT_CHANNEL_0_PORT + channel) << 4;
//     }

//     enable_interrupts();
// }
