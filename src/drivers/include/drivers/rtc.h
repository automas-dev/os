#ifndef DRIVER_RTC_H
#define DRIVER_RTC_H

#include <stdint.h>

typedef enum {
    RTC_RATE_32768_HZ = 1,  // 30.517578125 us
    RTC_RATE_16384_HZ = 2,  // 61.03515625 us
    RTC_RATE_8192_HZ  = 3,  // 122.0703125 us
    RTC_RATE_4096_HZ  = 4,  // 244.140625 us
    RTC_RATE_2048_HZ  = 5,  // 488.28125 us
    RTC_RATE_1024_HZ  = 6,  // 976.5625 us
    RTC_RATE_512_HZ   = 7,  // 1.953125 ms
    RTC_RATE_256_HZ   = 8,  // 3.90625 ms
    RTC_RATE_128_HZ   = 9,  // 7.8125 ms
    RTC_RATE_64_HZ    = 10, // 15.625 ms
    RTC_RATE_32_HZ    = 11, // 31.25 ms
    RTC_RATE_16_HZ    = 12, // 62.5 ms
    RTC_RATE_8_HZ     = 13, // 125.0 ms
    RTC_RATE_4_HZ     = 14, // 250.0 ms
    RTC_RATE_2_HZ     = 15, // 500.0 ms
} rtc_rate_t;

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

void init_rtc(rtc_rate_t rate);

// uint32_t time_us();
// uint32_t time_ms();
// uint32_t time_s();

rtc_time_t * rtc_time();

#endif // DRIVER_RTC_H
