#include "kernel/logs.h"

#include <stdarg.h>

#include "cpu/mmu.h"
#include "drivers/serial.h"
#include "kernel/time.h"
#include "libc/stdio.h"
#include "libc/stdio_impl.h"
#include "libc/string.h"

int __enabled;
int __time_enabled;
int __level;

static void put_time();

void kernel_log_init() {
    __enabled      = 1;
    __time_enabled = 0;
    __level        = 0;
}

void kernel_log_enable() {
    __enabled = 1;
}

void kernel_log_disable() {
    __enabled = 0;
}

void kernel_log_time_enable() {
    __time_enabled = 1;
}

void kernel_log_time_disable() {
    __time_enabled = 0;
}

void kernel_log_set_level(int level) {
    __level = level;
}

volatile int start_now = 0;

void kernel_log(int level, const char * file, size_t lineno, const char * service, const char * fmt, ...) {
    if (level < __level) {
        return;
    }

    if (__time_enabled) {
        put_time();
    }

    if (file) {
        printf("[%s:%u]", file, lineno);
    }

    if (service) {
        printf("[%s]", service);
    }

    serial_write_str(SERIAL_PORT_COM1, ": ");

    va_list params;
    va_start(params, fmt);
    vprintf(stdout, fmt, params);
    if (start_now) {
        serial_write_str(SERIAL_PORT_COM1, "\n");
    }
}

static void put_time() {
    uint32_t ms = time_ms();
    uint32_t s  = ms / 1e3;
    ms %= 1000;

    printf("[%3u.%03u]", s, ms);
}
