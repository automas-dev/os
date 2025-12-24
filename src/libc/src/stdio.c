#include "libc/stdio.h"

#include <stdarg.h>

#include "libc/stdio_impl.h"
#include "libc/string.h"
#include "libk/sys_call.h"

#ifndef TESTING

file_t _stdin = {
    .handle = 0,
    .flags  = FILE_FLAG_READ,
    .size   = 0,
    .pos    = 0,
};

file_t _stdout = {
    .handle = 1,
    .flags  = FILE_FLAG_WRITE,
    .size   = 0,
    .pos    = 0,
};

file_t _stderr = {
    .handle = 2,
    .flags  = FILE_FLAG_WRITE,
    .size   = 0,
    .pos    = 0,
};

size_t puts(const char * str) {
    return _sys_io_write(1, str, kstrlen(str), 0);
}

size_t putc(char c) {
    return _sys_io_write(1, &c, 1, 0);
}

size_t puti(int32_t num, uint8_t base, bool upper) {
    return vputi(stdout, num, base, upper);
}

size_t putli(int64_t num, uint8_t base, bool upper) {
    return vputli(stdout, num, base, upper);
}

size_t putu(uint32_t num, uint8_t base, bool upper) {
    return vputu(stdout, num, base, upper);
}

size_t putlu(uint64_t num, uint8_t base, bool upper) {
    return vputlu(stdout, num, base, upper);
}

char getc() {
    char c = 0;
    _sys_io_read(0, &c, 1, 0);
    return c;
}

size_t gets(size_t size, char * buff) {
    return _sys_io_read(stdin->handle, buff, size, 0);
}

size_t printf(const char * fmt, ...) {
    va_list params;
    va_start(params, fmt);
    return vprintf(stdout, fmt, params);
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
