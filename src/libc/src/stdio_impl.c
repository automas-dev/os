#include "libc/stdio_impl.h"

#include "libc/string.h"

static size_t vprintf_impl(puts_fn ps, putc_fn pc, const char * fmt, va_list params);

static size_t int_width(int32_t n, uint8_t base);
static size_t long_int_width(int64_t n, uint8_t base);
static size_t uint_width(uint32_t n, uint8_t base);
static size_t long_uint_width(uint64_t n, uint8_t base);

static char digit(uint32_t num, uint8_t base, bool upper);

static size_t pad(puts_fn ps, putc_fn pc, char c, size_t len);

static size_t padded_int(puts_fn ps, putc_fn pc, size_t width, bool left_align, int32_t num, uint8_t base, bool upper, bool lead_zero);
static size_t padded_long_int(puts_fn ps, putc_fn pc, size_t width, bool left_align, int64_t num, uint8_t base, bool upper, bool lead_zero);

static size_t padded_uint(puts_fn ps, putc_fn pc, size_t width, bool left_align, uint32_t num, uint8_t base, bool upper, bool lead_zero);
static size_t padded_long_uint(puts_fn ps, putc_fn pc, size_t width, bool left_align, uint64_t num, uint8_t base, bool upper, bool lead_zero);

static size_t padded_str(puts_fn ps, putc_fn pc, size_t width, bool left_align, char * str);

size_t vputi(puts_fn ps, putc_fn pc, int32_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return pc('0');
    }

    bool is_neg = num < 0;

    size_t o_len = 0;
    if (num < 0) {
        o_len += pc('-');
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
        o_len += pc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

size_t vputli(puts_fn ps, putc_fn pc, int64_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return pc('0');
    }

    bool is_neg = num < 0;

    size_t o_len = 0;
    if (num < 0) {
        o_len += pc('-');
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
        o_len += pc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

size_t vputu(puts_fn ps, putc_fn pc, uint32_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return pc('0');
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
        o_len += pc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

size_t vputlu(puts_fn ps, putc_fn pc, uint64_t num, uint8_t base, bool upper) {
    if (num == 0) {
        return pc('0');
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
        o_len += pc(digit(rev % base, base, upper));
        rev /= base;
    }

    return o_len;
}

size_t vprintf(puts_fn ps, putc_fn pc, const char * fmt, ...) {
    va_list params;
    va_start(params, fmt);
    return vprintf_impl(ps, pc, fmt, params);
}

static size_t vprintf_impl(puts_fn ps, putc_fn pc, const char * fmt, va_list params) {
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
                        o_len += padded_long_int(ps, pc, width, left_align, arg, 10, false, lead_zero);
                    }
                    else {
                        int32_t arg = va_arg(params, int);
                        o_len += padded_int(ps, pc, width, left_align, arg, 10, false, lead_zero);
                    }
                } break;
                case 'u': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, unsigned int);
                        o_len += padded_long_uint(ps, pc, width, left_align, arg, 10, false, lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, unsigned int);
                        o_len += padded_uint(ps, pc, width, left_align, arg, 10, false, lead_zero);
                    }
                } break;
                case 'p': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, unsigned int);
                        o_len += ps("0x");
                        o_len += padded_long_uint(ps, pc, width, left_align, arg, 16, false, true);
                    }
                    else {
                        uint32_t arg = va_arg(params, unsigned int);
                        o_len += ps("0x");
                        o_len += padded_uint(ps, pc, width, left_align, arg, 16, false, true);
                    }
                } break;
                case 'o': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, int);
                        o_len += padded_long_uint(ps, pc, width, left_align, arg, 8, false, lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, int);
                        o_len += padded_uint(ps, pc, width, left_align, arg, 8, false, lead_zero);
                    }
                } break;
                case 'x':
                case 'X': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, int);
                        o_len += padded_long_uint(ps, pc, width, left_align, arg, 16, *fmt == 'X', lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, int);
                        o_len += padded_uint(ps, pc, width, left_align, arg, 16, *fmt == 'X', lead_zero);
                    }
                } break;
                case 'c': {
                    char arg = va_arg(params, int);
                    o_len += pc(arg);
                } break;
                case 's': {
                    char * arg = va_arg(params, char *);
                    o_len += padded_str(ps, pc, width, left_align, arg);
                } break;
                case 'n': {
                    int * arg = va_arg(params, int *);
                    *arg      = width;
                } break;
                case 'b': {
                    int arg = va_arg(params, int);
                    o_len += ps(arg ? "true" : "false");
                } break;
                case 'f': {
                    float    arg   = va_arg(params, double);
                    uint32_t lhs   = (uint32_t)arg;
                    size_t   count = vputi(ps, pc, lhs, 10, false);
                    o_len += count;
                    o_len += pc('.');
                    float rem = arg - (float)lhs;
                    if (!fract) {
                        fract = 6;
                    }
                    size_t f_count = 0;
                    while ((!width || count++ < width) && f_count++ < fract) {
                        rem *= 10;
                        // if (rem == 0)
                        //     break;
                        vputu(ps, pc, (int)rem, 10, false);
                        rem -= (int)rem;
                    }
                } break;
                case '%': {
                    o_len += pc('%');
                } break;
                default:
                    break;
            }
            fmt++;
        }
        else {
            o_len += pc(*fmt++);
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

static size_t pad(puts_fn ps, putc_fn pc, char c, size_t len) {
    size_t o_len = 0;
    while (len) {
        o_len += pc(c);
        len--;
    }
    return o_len;
}

static size_t padded_int(puts_fn ps, putc_fn pc, size_t width, bool left_align, int32_t num, uint8_t base, bool upper, bool lead_zero) {
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
            o_len += pc('-');
        }
        o_len += pad(ps, pc, (lead_zero ? '0' : ' '), width - num_len);
        if (!lead_zero && is_neg) {
            o_len += pc('-');
        }
    }
    else if (is_neg) {
        o_len += pc('-');
    }

    o_len += vputi(ps, pc, num, base, upper);

    if (fill && left_align) {
        o_len += pad(ps, pc, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_long_int(puts_fn ps, putc_fn pc, size_t width, bool left_align, int64_t num, uint8_t base, bool upper, bool lead_zero) {
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
            o_len += pc('-');
        }
        o_len += pad(ps, pc, (lead_zero ? '0' : ' '), width - num_len);
        if (!lead_zero && is_neg) {
            o_len += pc('-');
        }
    }
    else if (is_neg) {
        o_len += pc('-');
    }

    o_len += vputli(ps, pc, num, base, upper);

    if (fill && left_align) {
        o_len += pad(ps, pc, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_uint(puts_fn ps, putc_fn pc, size_t width, bool left_align, uint32_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = uint_width(num, base);

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(ps, pc, (lead_zero ? '0' : ' '), width - num_len);
    }

    o_len += vputu(ps, pc, num, base, upper);

    if (fill && left_align) {
        o_len += pad(ps, pc, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_long_uint(puts_fn ps, putc_fn pc, size_t width, bool left_align, uint64_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = long_uint_width(num, base);

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(ps, pc, (lead_zero ? '0' : ' '), width - num_len);
    }

    o_len += vputu(ps, pc, num, base, upper);

    if (fill && left_align) {
        o_len += pad(ps, pc, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_str(puts_fn ps, putc_fn pc, size_t width, bool left_align, char * str) {
    size_t str_len = kstrlen(str);
    bool   fill    = width > str_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(ps, pc, ' ', width - str_len);
    }

    size_t len = ps(str);

    if (fill && left_align) {
        o_len += pad(ps, pc, ' ', width - str_len);
    }

    return o_len;
}
