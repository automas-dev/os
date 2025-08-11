#include "kernel/time.h"

#include "cpu/isr.h"
#include "cpu/ports.h"
#include "drivers/pit.h"
#include "ebus.h"
#include "kernel/logs.h"
#include "libc/datastruct/array.h"
#include "libc/proc.h"
#include "libc/stdio.h"

// https://wiki.osdev.org/PIT

#define BASE_FREQ 1193180

typedef struct _timer {
    int      id;
    uint32_t count;
} timer_t;

uint32_t __tick    = 0;
uint32_t __freq    = 0;
int      __next_id = 1;

arr_t timers; // timer_t

static void timer_callback(registers_t * regs) {
    __tick++;
    for (int i = 0; i < arr_size(&timers); i++) {
        timer_t * timer = arr_at(&timers, i);
        timer->count--;
        if (timer->count == 0) {
            ebus_event_t e;
            e.event_id   = EBUS_EVENT_TIMER;
            e.timer.id   = timer->id;
            e.timer.time = __tick;
            queue_event(&e);
            arr_remove(&timers, i, 0);
            i--; // Account for the reduced size after insert
        }
    }
}

void init_time(uint32_t freq) {
    __tick    = 0;
    __freq    = freq;
    __next_id = 1;

    init_pit();

    if (arr_create(&timers, 4, sizeof(timer_t))) {
        return;
    }

    /* Install the function we just wrote */
    register_interrupt_handler(IRQ0, timer_callback);

    /* Get the PIT value: hardware clock at 1193180 Hz */
    uint32_t divisor = BASE_FREQ / freq;

    // pit_write_channel(0, PIT_ACCESS_MODE_LOW_HIGH, PIT_CHANNEL_MODE_3_SQUARE_WAVE_GEN, divisor);
    pit_write_channel(0, PIT_ACCESS_MODE_LOW_HIGH, PIT_CHANNEL_MODE_2_RATE_GEN, divisor);
}

int start_timer(uint32_t ticks) {
    timer_t t;
    t.id    = __next_id++;
    t.count = ticks;
    if (arr_insert(&timers, arr_size(&timers), &t)) {
        return -1;
    }
    return t.id;
}

int start_timer_ns(uint32_t ns) {
    return start_timer(ns * __freq / 1000000000);
}

int start_timer_ms(uint32_t ms) {
    return start_timer(ms * __freq / 1000);
}

void stop_timer(int id) {
    for (int i = 0; i < arr_size(&timers); i++) {
        timer_t * t = arr_at(&timers, i);
        if (t->id == id) {
            arr_remove(&timers, i, 0);
            return;
        }
    }
}

void sleep(uint32_t ms) {
    uint32_t goal = time_ms() + ms;
    while (time_ms() < goal) {
        asm("hlt");
    }
}

uint32_t time_ticks() {
    return __tick;
}

uint32_t time_s() {
    return __tick / __freq;
}

uint32_t time_ms() {
    return (__tick * 1e3) / __freq;
}

uint32_t time_us() {
    return (__tick * 1e6) / __freq;
}

uint64_t time_ns() {
    return ((uint64_t)__tick * 1e9) / (uint64_t)__freq;
}
