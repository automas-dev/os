#include "drivers/rtc.h"

#include <stdbool.h>

#include "cpu/isr.h"
#include "cpu/ports.h"

#define RTC_REG_PORT  0x70
#define RTC_DATA_PORT 0x71

#define RTC_FLAG_DISABLE_NMI 0x80
#define RTC_REG_A            0xa
#define RTC_REG_B            0xb
#define RTC_REG_C            0xc

static uint32_t ticks = 0;
uint32_t        frequency;

static rtc_time_t time;

static bool    read_in_progress();
static uint8_t read_rtc(uint8_t reg);

// uint32_t time_us() {
//     return ticks * 1e6 / frequency;
// }

// uint32_t time_ms() {
//     return ticks * 1e3 / frequency;
// }

// uint32_t time_s() {
//     return ticks / frequency;
// }

static void rtc_callback(registers_t * regs) {
    port_byte_out(RTC_REG_PORT, RTC_REG_C);
    port_byte_in(RTC_DATA_PORT);
    ticks++;
}

void init_rtc(rtc_rate_t rate) {
    register_interrupt_handler(IRQ8, rtc_callback);

    disable_interrupts();
    port_byte_out(RTC_REG_PORT, RTC_REG_B | RTC_FLAG_DISABLE_NMI);
    uint8_t prev = port_byte_in(RTC_DATA_PORT);
    port_byte_out(RTC_REG_PORT, RTC_REG_B | RTC_FLAG_DISABLE_NMI);
    port_byte_out(RTC_DATA_PORT, prev | 0x40);

    rate &= 0xF;
    port_byte_out(RTC_REG_PORT, RTC_REG_A | RTC_FLAG_DISABLE_NMI);
    prev = port_byte_in(RTC_DATA_PORT);
    port_byte_out(RTC_REG_PORT, RTC_REG_A | RTC_FLAG_DISABLE_NMI);
    port_byte_out(RTC_DATA_PORT, (prev & 0xF0) | rate);
    enable_interrupts();

    frequency = 32768 >> (rate - 1);
}

rtc_time_t * rtc_time() {
    while (read_in_progress());

    time.second = read_rtc(0x00);
    time.minute = read_rtc(0x02);
    time.hour   = read_rtc(0x04);
    time.day    = read_rtc(0x07);
    time.month  = read_rtc(0x08);
    time.year   = read_rtc(0x09);

    // TODO there's a lot more

    return &time;
}

static bool read_in_progress() {
    port_byte_out(RTC_REG_PORT, 0xA);
    return port_byte_in(RTC_DATA_PORT) & 0x80;
}

static uint8_t read_rtc(uint8_t reg) {
    port_byte_out(RTC_REG_PORT, reg);
    return port_byte_in(RTC_DATA_PORT);
}
