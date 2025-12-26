#include "libc/stdio_impl.h"

#include "libc/file.h"
#include "libc/string.h"

static size_t int_width(int32_t n, uint8_t base);
static size_t long_int_width(int64_t n, uint8_t base);
static size_t uint_width(uint32_t n, uint8_t base);
static size_t long_uint_width(uint64_t n, uint8_t base);

static char digit(uint32_t num, uint8_t base, bool upper);

static size_t pad(file_t * file, char c, size_t len);

static size_t padded_int(file_t * file, size_t width, bool left_align, int32_t num, uint8_t base, bool upper, bool lead_zero);
static size_t padded_long_int(file_t * file, size_t width, bool left_align, int64_t num, uint8_t base, bool upper, bool lead_zero);

static size_t padded_uint(file_t * file, size_t width, bool left_align, uint32_t num, uint8_t base, bool upper, bool lead_zero);
static size_t padded_long_uint(file_t * file, size_t width, bool left_align, uint64_t num, uint8_t base, bool upper, bool lead_zero);

static size_t padded_str(file_t * file, size_t width, bool left_align, char * str);

size_t vputi(file_t * file, int32_t num, uint8_t base, bool upper) {
    if (!file || base < 8) {
        return 0;
    }

    // 32 bit oct = 11 + - + null terminator
    char   buff[13];
    size_t buff_len = itoa_base(sizeof(buff), num, buff, base, upper);

    return file_write(file, 1, buff_len, buff);
}

size_t vputli(file_t * file, int64_t num, uint8_t base, bool upper) {
    if (!file || base < 8) {
        return 0;
    }

    // 32 bit oct = 24 + - + null terminator
    char   buff[26];
    size_t buff_len = ltoa_base(sizeof(buff), num, buff, base, upper);

    return file_write(file, 1, buff_len, buff);
}

size_t vputu(file_t * file, uint32_t num, uint8_t base, bool upper) {
    if (!file || base < 8) {
        return 0;
    }

    // 32 bit oct = 11 + - + null terminator
    char   buff[13];
    size_t buff_len = utoa_base(sizeof(buff), num, buff, base, upper);

    return file_write(file, 1, buff_len, buff);
}

size_t vputlu(file_t * file, uint64_t num, uint8_t base, bool upper) {
    if (!file || base < 8) {
        return 0;
    }

    // 32 bit oct = 24 + - + null terminator
    char   buff[26];
    size_t buff_len = ultoa_base(sizeof(buff), num, buff, base, upper);

    return file_write(file, 1, buff_len, buff);
}

size_t vputs(file_t * file, const char * str) {
    if (!file || !str) {
        return 0;
    }

    return file_write(file, 1, kstrlen(str), str);
}

size_t vaprintf(file_t * file, const char * fmt, ...) {
    va_list params;
    va_start(params, fmt);
    return vprintf(file, fmt, params);
}

size_t vprintf(file_t * file, const char * fmt, va_list params) {
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
                        o_len += padded_long_int(file, width, left_align, arg, 10, false, lead_zero);
                    }
                    else {
                        int32_t arg = va_arg(params, int);
                        o_len += padded_int(file, width, left_align, arg, 10, false, lead_zero);
                    }
                } break;
                case 'u': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, unsigned int);
                        o_len += padded_long_uint(file, width, left_align, arg, 10, false, lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, unsigned int);
                        o_len += padded_uint(file, width, left_align, arg, 10, false, lead_zero);
                    }
                } break;
                case 'p': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, unsigned int);
                        o_len += file_write(file, 1, 2, "0x");
                        o_len += padded_long_uint(file, width, left_align, arg, 16, false, true);
                    }
                    else {
                        uint32_t arg = va_arg(params, unsigned int);
                        o_len += file_write(file, 1, 2, "0x");
                        o_len += padded_uint(file, width, left_align, arg, 16, false, true);
                    }
                } break;
                case 'o': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, int);
                        o_len += padded_long_uint(file, width, left_align, arg, 8, false, lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, int);
                        o_len += padded_uint(file, width, left_align, arg, 8, false, lead_zero);
                    }
                } break;
                case 'x':
                case 'X': {
                    if (is_long) {
                        uint64_t arg = va_arg(params, int);
                        o_len += padded_long_uint(file, width, left_align, arg, 16, *fmt == 'X', lead_zero);
                    }
                    else {
                        uint32_t arg = va_arg(params, int);
                        o_len += padded_uint(file, width, left_align, arg, 16, *fmt == 'X', lead_zero);
                    }
                } break;
                case 'c': {
                    char arg = va_arg(params, int);
                    o_len += file_write(file, 1, 1, &arg);
                } break;
                case 's': {
                    char * arg = va_arg(params, char *);
                    o_len += padded_str(file, width, left_align, arg);
                } break;
                case 'n': {
                    int * arg = va_arg(params, int *);
                    *arg      = width;
                } break;
                case 'b': {
                    int arg = va_arg(params, int);
                    if (arg) {
                        o_len += file_write(file, 1, 4, "true");
                    }
                    else {
                        o_len += file_write(file, 1, 5, "false");
                    }
                } break;
                case 'f': {
                    float    arg   = va_arg(params, double);
                    uint32_t lhs   = (uint32_t)arg;
                    size_t   count = vputi(file, lhs, 10, false);
                    o_len += count;
                    o_len += file_write(file, 1, 1, ".");
                    float rem = arg - (float)lhs;
                    if (!fract) {
                        fract = 6;
                    }
                    size_t f_count = 0;
                    while ((!width || count++ < width) && f_count++ < fract) {
                        rem *= 10;
                        // if (rem == 0)
                        //     break;
                        vputu(file, (int)rem, 10, false);
                        rem -= (int)rem;
                    }
                } break;
                case '%': {
                    o_len += file_write(file, 1, 1, "%");
                } break;
                default:
                    break;
            }
            fmt++;
        }
        else {
            o_len += file_write(file, 1, 1, fmt++);
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

static size_t pad(file_t * file, char c, size_t len) {
    size_t o_len = 0;
    while (len) {
        o_len += file_write(file, 1, 1, &c);
        len--;
    }
    return o_len;
}

static size_t padded_int(file_t * file, size_t width, bool left_align, int32_t num, uint8_t base, bool upper, bool lead_zero) {
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
            o_len += file_write(file, 1, 1, "-");
        }
        o_len += pad(file, (lead_zero ? '0' : ' '), width - num_len);
        if (!lead_zero && is_neg) {
            o_len += file_write(file, 1, 1, "-");
        }
    }
    else if (is_neg) {
        o_len += file_write(file, 1, 1, "-");
    }

    o_len += vputi(file, num, base, upper);

    if (fill && left_align) {
        o_len += pad(file, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_long_int(file_t * file, size_t width, bool left_align, int64_t num, uint8_t base, bool upper, bool lead_zero) {
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
            o_len += file_write(file, 1, 1, "-");
        }
        o_len += pad(file, (lead_zero ? '0' : ' '), width - num_len);
        if (!lead_zero && is_neg) {
            o_len += file_write(file, 1, 1, "-");
        }
    }
    else if (is_neg) {
        o_len += file_write(file, 1, 1, "-");
    }

    o_len += vputli(file, num, base, upper);

    if (fill && left_align) {
        o_len += pad(file, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_uint(file_t * file, size_t width, bool left_align, uint32_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = uint_width(num, base);

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(file, (lead_zero ? '0' : ' '), width - num_len);
    }

    o_len += vputu(file, num, base, upper);

    if (fill && left_align) {
        o_len += pad(file, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_long_uint(file_t * file, size_t width, bool left_align, uint64_t num, uint8_t base, bool upper, bool lead_zero) {
    size_t num_len = long_uint_width(num, base);

    bool fill = width > num_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(file, (lead_zero ? '0' : ' '), width - num_len);
    }

    o_len += vputu(file, num, base, upper);

    if (fill && left_align) {
        o_len += pad(file, ' ', width - num_len);
    }

    return o_len;
}

static size_t padded_str(file_t * file, size_t width, bool left_align, char * str) {
    size_t str_len = kstrlen(str);
    bool   fill    = width > str_len;

    size_t o_len = 0;
    if (fill && !left_align) {
        o_len += pad(file, ' ', width - str_len);
    }

    size_t len = file_write(file, 1, str_len, str);

    if (fill && left_align) {
        o_len += pad(file, ' ', width - str_len);
    }

    return o_len;
}
