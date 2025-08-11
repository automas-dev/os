#include "kernel/logs.h"

#include <stdarg.h>

#include "drivers/vga.h"
#include "kernel/time.h"
#include "libc/stdio_impl.h"
#include "libc/string.h"

static void put_time();

void kernel_log(int level, const char * service, const char * fmt, ...) {
    if (level < 0) {
        return;
    }

    vga_color(VGA_FG_GREEN | VGA_BG_BLACK);
    put_time();

    if (service) {
        vga_color(VGA_FG_MAGENTA);
        vga_puts(service);
        vga_puts(": ");
    }

    // Message color
    switch (level) {
        case KERNEL_LOG_LEVEL_TRACE:
            vga_color(VGA_FG_CYAN | VGA_BG_BLACK);
            break;
        default:
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
    vprintf(vga_puts, vga_putc, fmt, params);
    vga_color(VGA_RESET);
    vga_putc('\n');
}

static void put_time() {
    uint32_t ms = time_ms();
    uint32_t s  = ms / 1e3;
    ms %= 1000;

    vprintf(vga_puts, vga_putc, "[%3u.%03u] ", s, ms);
}
