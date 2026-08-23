#define KLOG_SERVICE "KERNEL/LOGS"

#include "kernel/logs.h"

#include <stdarg.h>

#include "config.h"
#include "cpu/mmu.h"
#include "drivers/serial.h"
#include "kernel/time.h"
#include "libc/stdio.h"
#include "libc/stdio_impl.h"
#include "libc/string.h"

static int __enabled;
static int __time_enabled;
static int __level;

static void put_time();

static const char * KERNEL_LOG_LEVEL_NAME[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
};

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

volatile int kernel_logs_enable_serial_newlines = 0;

void kernel_log(int level, const char * file, size_t lineno, const char * service, const char * fmt, ...) {
    if (!__enabled) {
        return;
    }

    if (level < __level) {
        return;
    }

    if (__time_enabled) {
        put_time();
    }
    // else {
    //     puts("[0.000]");
    // }

    // Bounds check for name lookup
    if (level < 0) {
        level = 0;
    }
    else if (level >= KERNEL_LOG_LEVEL__LENGTH) {
        level = KERNEL_LOG_LEVEL__LENGTH - 1;
    }

    printf("[%s]", KERNEL_LOG_LEVEL_NAME[level]);

    // if (file) {
    //     if (kstrlen(file) > FILE_PREFIX_LENGTH) {
    //         file += FILE_PREFIX_LENGTH;
    //     }
    //     printf("[%s:%u]", file, lineno);
    // }

    if (service) {
        // printf("[%s]", service);
        printf("[%s:%u]", service, lineno);
    }

    puts(": ");

    va_list params;
    va_start(params, fmt);
    vprintf(stdout, fmt, params);
    // Printing a newline to serial during mmu / gdt / tss blocks the kernel, idk why
    if (kernel_logs_enable_serial_newlines) {
        putc('\n');
    }
}

static void put_time() {
    uint32_t ms = time_ms();
    uint32_t s  = ms / 1e3;
    ms %= 1000;

    // printf("[%3u.%03u]", s, ms);
    printf("[%u.%03u]", s, ms);
}
