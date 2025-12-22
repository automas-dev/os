#include "kernel/logs.h"

#include <stdarg.h>

#include "kernel/time.h"
#include "libc/stdio.h"
#include "libc/stdio_impl.h"
#include "libc/string.h"
#include "drivers/vga.h"

int __enabled;
int __time_enabled;
int __level;

static void put_time();

void init_kernel_logs() {
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

void kernel_log(int level, const char * service, const char * fmt, ...) {
    if (level < __level) {
        return;
    }

    if (__time_enabled) {
        vga_color(VGA_FG_GREEN | VGA_BG_BLACK);
        put_time();
    }

    vga_color(VGA_RESET);

    if (service) {
        vga_color(VGA_FG_MAGENTA);
        vga_puts(service);
        vga_puts(": ");
    }

    // Message color
    switch (level) {
        default:
        case KERNEL_LOG_LEVEL_TRACE:
            vga_color(VGA_FG_CYAN | VGA_BG_BLACK);
            break;
        case KERNEL_LOG_LEVEL_DEBUG:
            vga_color(VGA_FG_LIGHT_GRAY | VGA_BG_BLACK);
            break;
        case KERNEL_LOG_LEVEL_INFO:
            vga_color(VGA_FG_WHITE | VGA_BG_BLACK);
            break;
        case KERNEL_LOG_LEVEL_WARNING:
            vga_color(VGA_FG_LIGHT_BROWN | VGA_BG_BLACK);
            break;
        case KERNEL_LOG_LEVEL_ERROR:
            vga_color(VGA_FG_LIGHT_RED | VGA_BG_BLACK);
            break;
    };

    va_list params;
    va_start(params, fmt);
    vprintf(stdout, fmt, params);
    vga_color(VGA_RESET);
    vga_putc('\n');
}

static void put_time() {
    uint32_t ms = time_ms();
    uint32_t s  = ms / 1e3;
    ms %= 1000;

    printf("[%3u.%03u] ", s, ms);
}
