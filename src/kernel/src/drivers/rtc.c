#define KLOG_SERVICE "DRIVERS/RTC"

#include "drivers/rtc.h"

#include <stdbool.h>

#include "cpu/isr.h"
#include "cpu/ports.h"
#include "kernel/logs.h"

#define RTC_REG_PORT  0x70
#define RTC_DATA_PORT 0x71

#define RTC_FLAG_DISABLE_NMI 0x80
#define RTC_REG_A            0xa
#define RTC_REG_B            0xb
#define RTC_REG_C            0xc

static uint32_t __ticks = 0;
static uint32_t __frequency;

static rtc_time_t __time;

static bool    read_in_progress();
static uint8_t read_rtc(uint8_t reg);

// uint32_t time_us() {
//     return __ticks * 1e6 / __frequency;
// }

// uint32_t time_ms() {
//     return __ticks * 1e3 / __frequency;
// }

// uint32_t time_s() {
//     return __ticks / __frequency;
// }

static void rtc_callback(registers_t * regs) {
    // TODO log registers
    KLOG_TRACE("rtc callback");
    port_byte_out(RTC_REG_PORT, RTC_REG_C);
    port_byte_in(RTC_DATA_PORT);
    __ticks++;
}

void rtc_init(rtc_rate_t rate) {
    KLOG_DEBUG("Registering interrupt handler on IRQ 8");
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

    __frequency = 32768 >> (rate - 1);
    KLOG_DEBUG("RTC frequency is %u hz", __frequency);

    KLOG_DEBUG("Initialized driver");
}

rtc_time_t * rtc_time() {
    if (read_in_progress()) {
        KLOG_TRACE("Waiting for read in progress");
        while (read_in_progress());
        KLOG_TRACE("Finished for read in progress");
    }

    __time.second = read_rtc(0x00);
    __time.minute = read_rtc(0x02);
    __time.hour   = read_rtc(0x04);
    __time.day    = read_rtc(0x07);
    __time.month  = read_rtc(0x08);
    __time.year   = read_rtc(0x09);

    // TODO there's a lot more

    // TODO this is wrong
    KLOG_TRACE("Time is %u-%02u-%02u %02u:%02u:%02u", __time.year, __time.month, __time.day, __time.hour, __time.minute, __time.second);

    return &__time;
}

static bool read_in_progress() {
    port_byte_out(RTC_REG_PORT, 0xA);
    return port_byte_in(RTC_DATA_PORT) & 0x80;
}

static uint8_t read_rtc(uint8_t reg) {
    port_byte_out(RTC_REG_PORT, reg);
    return port_byte_in(RTC_DATA_PORT);
}
