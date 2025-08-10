#include "kernel/logs.h"

#include <stdarg.h>

#include "drivers/timer.h"
#include "drivers/vga.h"
#include "libc/stdio.h"
#include "libc/string.h"

#define S_WIDTH  3
#define MS_WIDTH 3

static void put_time();

static size_t kputi(int32_t num, uint8_t base, bool upper);
static size_t kputli(int64_t num, uint8_t base, bool upper);
static size_t kputu(uint32_t num, uint8_t base, bool upper);
static size_t kputlu(uint64_t num, uint8_t base, bool upper);
static size_t kvprintf(const char * fmt, va_list params);

static size_t int_width(int32_t n, uint8_t base);
static size_t long_int_width(int64_t n, uint8_t base);
static size_t uint_width(uint32_t n, uint8_t base);
static size_t long_uint_width(uint64_t n, uint8_t base);

static char digit(uint32_t num, uint8_t base, bool upper);

static size_t pad(char c, size_t len);

static size_t padded_int(size_t width, bool left_align, int32_t num, uint8_t base, bool upper, bool lead_zero);
static size_t padded_long_int(size_t width, bool left_align, int64_t num, uint8_t base, bool upper, bool lead_zero);

static size_t padded_uint(size_t width, bool left_align, uint32_t num, uint8_t base, bool upper, bool lead_zero);
static size_t padded_long_uint(size_t width, bool left_align, uint64_t num, uint8_t base, bool upper, bool lead_zero);

static size_t padded_str(size_t width, bool left_align, char * str);

void kernel_log(const char * fmt, ...) {
    vga_color(VGA_RESET);
    vga_color(VGA_FG_GREEN);
    put_time();

    vga_color(VGA_WHITE_ON_BLACK);
    va_list params;
    va_start(params, fmt);
    vprintf(fmt, params);
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
    vprintf(fmt, params);
    vga_color(VGA_RESET);
    vga_putc('\n');
}

static void put_time() {
    uint32_t us = get_time_us();
    uint32_t s = us / 1e6;
    us %= 1000000;

    vga_putc('[');
    // putlu(ns, 10, false);
    padded_uint(S_WIDTH, false, s, 10, false, false);
    vga_putc('.');
    padded_uint(6, false, us, 10, false, true);
    vga_puts("] ");
}

static size_t kputi(int32_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return vga_putc('0');
    }

    bool is_neg = num < 0;

    size_t o_len = 0;
    if (num < 0) {
        o_len += vga_putc('-');
        num = -num;
    }

    size_t   len = 0;
    uint32_t rev = 0;
    while (num > 0) {
        rev = (rev * base) + (num % base);
        num /= base;
        len++;
    }

    for (size_t i = 0; i < len; i++) {
        o_len += vga_putc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

static size_t kputli(int64_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return vga_putc('0');
    }

    bool is_neg = num < 0;

    size_t o_len = 0;
    if (num < 0) {
        o_len += vga_putc('-');
        num = -num;
    }

    size_t   len = 0;
    uint64_t rev = 0;
    while (num > 0) {
        rev = (rev * base) + (num % base);
        num /= base;
        len++;
    }

    for (size_t i = 0; i < len; i++) {
        o_len += vga_putc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

static size_t kputu(uint32_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return vga_putc('0');
    }

    size_t   len = 0;
    uint32_t rev = 0;
    while (num > 0) {
        rev = (rev * base) + (num % base);
        num /= base;
        len++;
    }

    size_t o_len = 0;
    for (size_t i = 0; i < len; i++) {
        o_len += vga_putc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

static size_t kputlu(uint64_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return vga_putc('0');
    }

    size_t   len = 0;
    uint64_t rev = 0;
    while (num > 0) {
        rev = (rev * base) + (num % base);
        num /= base;
        len++;
    }

    size_t o_len = 0;
    for (size_t i = 0; i < len; i++) {
        o_len += vga_putc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

static size_t kvprintf(const char * fmt, va_list params) {
    size_t o_len = 0;
    while (*fmt) {
        if (*fmt == '%') {
            size_t width      = 0;
            size_t fract      = 0;
            bool   fill_fract = false;
            bool   left_align = fmt[1] == '-';
            bool   lead_zero  = !left_align && fmt[1] == '0';
            bool   is_long    = false;

            if (left_align || lead_zero) {
                fmt++;
            }

        start_format:
            fmt++;
            switch (*fmt) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    if (!fill_fract) {
                        width = width * 10 + (*fmt - '0');
                    }
                    else {
                        fract = fract * 10 + (*fmt - '0');
                    }
                    goto start_format;
                case '.':
                    fill_fract = true;
                    goto start_format;
                case 'l': {
                    is_long = true;
                    goto start_format;
                }
                case 'd': {
                    if (is_long) {
                        int64_t arg = va_arg(params, int);
                        o_len += padded_long_int(width, left_align, arg, 10, false, lead_zero);
                    }
                    else {
                        int32_t arg = va_arg(params, int);
                        o_len += padded_int(width, left_align, arg, 10, false, lead_zero);
                    }
                } break;
                case 'u': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, unsigned int);
                        o_len += padded_long_uint(width, left_align, arg, 10, false, lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, unsigned int);
                        o_len += padded_uint(width, left_align, arg, 10, false, lead_zero);
                    }
                } break;
                case 'p': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, unsigned int);
                        o_len += vga_puts("0x");
                        o_len += padded_long_uint(width, left_align, arg, 16, false, true);
                    }
                    else {
                        uint32_t arg = va_arg(params, unsigned int);
                        o_len += vga_puts("0x");
                        o_len += padded_uint(width, left_align, arg, 16, false, true);
                    }
                } break;
                case 'o': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, int);
                        o_len += padded_long_uint(width, left_align, arg, 8, false, lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, int);
                        o_len += padded_uint(width, left_align, arg, 8, false, lead_zero);
                    }
                } break;
                case 'x':
                case 'X': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, int);
                        o_len += padded_long_uint(width, left_align, arg, 16, *fmt == 'X', lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, int);
                        o_len += padded_uint(width, left_align, arg, 16, *fmt == 'X', lead_zero);
                    }
                } break;
                case 'c': {
                    char arg = va_arg(params, int);
                    o_len += vga_putc(arg);
                } break;
                case 's': {
                    char * arg = va_arg(params, char *);
                    o_len += padded_str(width, left_align, arg);
                } break;
                case 'n': {
                    int * arg = va_arg(params, int *);
                    *arg      = width;
                } break;
                case 'b': {
                    int arg = va_arg(params, int);
                    o_len += vga_puts(arg ? "true" : "false");
                } break;
                case 'f': {
                    float    arg   = va_arg(params, double);
                    uint32_t lhs   = (uint32_t)arg;
                    size_t   count = puti(lhs, 10, false);
                    o_len += count;
                    o_len += vga_putc('.');
                    float rem = arg - (float)lhs;
                    if (!fract) {
                        fract = 6;
                    }
                    size_t f_count = 0;
                    while ((!width || count++ < width) && f_count++ < fract) {
                        rem *= 10;
                        // if (rem == 0)
                        //     break;
                        putu((int)rem, 10, false);
                        rem -= (int)rem;
                    }
                } break;
                case '%': {
                    o_len += vga_putc('%');
                } break;
                default:
                    break;
            }
            fmt++;
        }
        else {
            o_len += vga_putc(*fmt++);
        };
    }

    return o_len;
}

static size_t int_width(int32_t n, uint8_t base) {
    if (n < 0) {
        n = -n;
    }
    return uint_width(n, base);
}

static size_t long_int_width(int64_t n, uint8_t base) {
    if (n < 0) {
        n = -n;
    }
    return long_uint_width(n, base);
}

static size_t uint_width(uint32_t n, uint8_t base) {
    size_t width = 0;
    while (n > 0) {
        n /= base;
        width++;
    }
    return (width ? width : 1);
}

static size_t long_uint_width(uint64_t n, uint8_t base) {
    size_t width = 0;
    while (n > 0) {
        n /= base;
        width++;
    }
    return (width ? width : 1);
}

static char digit(uint32_t num, uint8_t base, bool upper) {
    if (num < 10) {
        return num + '0';
    }
    else {
        return (num - 10) + (upper ? 'A' : 'a');
    }
}

static size_t pad(char c, size_t len) {
    size_t o_len = 0;
    while (len) {
        o_len += vga_putc(c);
        len--;
    }
    return o_len;
}

static size_t padded_int(size_t width, bool left_align, int32_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = int_width(num, base);
    bool   is_neg  = num < 0;

    if (is_neg) {
        num_len++;
        num = -num;
    }

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        if (lead_zero && is_neg) {
            o_len += vga_putc('-');
        }
        o_len += pad((lead_zero ? '0' : ' '), width - num_len);
        if (!lead_zero && is_neg) {
            o_len += vga_putc('-');
        }
    }
    else if (is_neg) {
        o_len += vga_putc('-');
    }

    o_len += puti(num, base, upper);

    if (fill && left_align) {
        o_len += pad(' ', width - num_len);
    }

    return o_len;
}

static size_t padded_long_int(size_t width, bool left_align, int64_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = long_int_width(num, base);
    bool   is_neg  = num < 0;

    if (is_neg) {
        num_len++;
        num = -num;
    }

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        if (lead_zero && is_neg) {
            o_len += vga_putc('-');
        }
        o_len += pad((lead_zero ? '0' : ' '), width - num_len);
        if (!lead_zero && is_neg) {
            o_len += vga_putc('-');
        }
    }
    else if (is_neg) {
        o_len += vga_putc('-');
    }

    o_len += putli(num, base, upper);

    if (fill && left_align) {
        o_len += pad(' ', width - num_len);
    }

    return o_len;
}

static size_t padded_uint(size_t width, bool left_align, uint32_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = uint_width(num, base);

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad((lead_zero ? '0' : ' '), width - num_len);
    }

    o_len += putu(num, base, upper);

    if (fill && left_align) {
        o_len += pad(' ', width - num_len);
    }

    return o_len;
}

static size_t padded_long_uint(size_t width, bool left_align, uint64_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = long_uint_width(num, base);

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad((lead_zero ? '0' : ' '), width - num_len);
    }

    o_len += putu(num, base, upper);

    if (fill && left_align) {
        o_len += pad(' ', width - num_len);
    }

    return o_len;
}

static size_t padded_str(size_t width, bool left_align, char * str) {
    size_t str_len = kstrlen(str);
    bool   fill    = width > str_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(' ', width - str_len);
    }

    size_t len = vga_puts(str);

    if (fill && left_align) {
        o_len += pad(' ', width - str_len);
    }

    return o_len;
}
