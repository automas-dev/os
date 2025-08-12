#include "libc/stdio.h"

#include <stdarg.h>

#include "libc/stdio_impl.h"
#include "libc/string.h"
#include "libk/sys_call.h"

#ifndef TESTING

size_t itoa(int32_t n, char * str) {
    bool is_neg = n < 0;

    if (is_neg) {
        *str++ = '-';

        n = -n;
    }

    size_t   len = 0;
    uint32_t rev = 0;
    while (n > 0) {
        rev = (rev * 10) + (n % 10);
        n /= 10;
        len += 1;
    }

    for (size_t i = 0; i < len; i++) {
        *str++ = '0' + (rev % 10);
        rev /= 10;
    }

    if (len == 0) {
        *str++ = '0';
        len++;
    }

    *str = 0;

    if (is_neg) {
        len++;
    }

    return len;
}

size_t ltoa(int64_t n, char * str) {
    bool is_neg = n < 0;

    if (is_neg) {
        *str++ = '-';

        n = -n;
    }

    size_t   len = 0;
    uint64_t rev = 0;
    while (n > 0) {
        rev = (rev * 10) + (n % 10);
        n /= 10;
        len += 1;
    }

    for (size_t i = 0; i < len; i++) {
        *str++ = '0' + (rev % 10);
        rev /= 10;
    }

    if (len == 0) {
        *str++ = '0';
        len++;
    }

    *str = 0;

    if (is_neg) {
        len++;
    }

    return len;
}

size_t puts(const char * str) {
    return _sys_puts(str);
}

size_t putc(char c) {
    return _sys_putc(c);
}

size_t puti(int32_t num, uint8_t base, bool upper) {
    return vputi(_sys_puts, _sys_putc, num, base, upper);
}

size_t putli(int64_t num, uint8_t base, bool upper) {
    return vputli(_sys_puts, _sys_putc, num, base, upper);
}

size_t putu(uint32_t num, uint8_t base, bool upper) {
    return vputu(_sys_puts, _sys_putc, num, base, upper);
}

size_t putlu(uint64_t num, uint8_t base, bool upper) {
    return vputlu(_sys_puts, _sys_putc, num, base, upper);
}

size_t printf(const char * fmt, ...) {
    va_list params;
    va_start(params, fmt);
    return vprintf(_sys_puts, _sys_putc, fmt, params);
}

size_t print_hexblock(const uint8_t * data, size_t count, size_t addr_offset) {
    size_t step  = 16;
    size_t o_len = 0;
    size_t line  = 0;
    if (!addr_offset) {
        o_len += puts("       00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n");
        o_len += puts("       -----------------------------------------------\n");
    }
    while (count) {
        o_len += printf("0x%04X ", line * step + addr_offset);
        size_t to_write = step;
        if (count < to_write) {
            to_write = count;
        }
        for (size_t i = 0; i < to_write; i++) {
            o_len += printf("%02X ", data[line * step + i]);
        }
        size_t space = step - to_write;
        while (space--) {
            o_len += puts("   ");
        }

        o_len += puts("| ");
        for (size_t i = 0; i < to_write; i++) {
            char c = data[line * step + i];
            if (c < 32) {
                c = '.';
            }
            o_len += putc(c);
        }
        space = step - to_write;
        while (space--) {
            o_len += putc(' ');
        }
        o_len += puts(" |\n");
        if (count <= step) {
            break;
        }
        count -= step;
        line++;
    }
    return o_len;
}

#endif
