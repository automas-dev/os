#include "kernel/logs.h"

#include <stdarg.h>

#include "drivers/vga.h"
#include "kernel/time.h"
#include "libc/stdio_impl.h"
#include "libc/string.h"

static void put_time();

void kernel_log(const char * fmt, ...) {
    vga_color(VGA_RESET);
    vga_color(VGA_FG_GREEN);
    put_time();

    vga_color(VGA_WHITE_ON_BLACK);
    va_list params;
    va_start(params, fmt);
    vprintf(vga_puts, vga_putc, fmt, params);
    vga_color(VGA_RESET);
    vga_putc('\n');
}

void kernel_service_log(const char * service, const char * fmt, ...) {
    vga_color(VGA_RESET);
    vga_color(VGA_FG_GREEN);
    put_time();

    vga_color(VGA_FG_BROWN);
    vga_puts(service);
    vga_puts(": ");

    vga_color(VGA_WHITE_ON_BLACK);
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
