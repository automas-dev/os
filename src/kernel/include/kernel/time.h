#ifndef KERNEL_TIME_H
#define KERNEL_TIME_H

#include <stdint.h>

enum TIMER_FREQ {
    TIMER_FREQ_S  = 1,
    TIMER_FREQ_MS = 1000,
    TIMER_FREQ_US = 1000000,
};

void time_init(uint32_t freq);

/**
 * @brief
 *
 * @param ticks
 * @return int id, < 0 for fail
 */
int time_start_timer(uint32_t ticks);
int time_start_timer_ns(uint32_t ns);
int time_start_timer_ms(uint32_t ms);

void time_stop_timer(int id);

void sleep(uint32_t ms);

uint32_t time_ticks();

uint32_t time_s();
uint32_t time_ms();
uint32_t time_us();
uint64_t time_ns();

#endif // KERNEL_TIME_H
