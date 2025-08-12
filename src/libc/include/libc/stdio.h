#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef TESTING

size_t itoa(int32_t n, char * str);
size_t ltoa(int64_t n, char * str);

size_t puts(const char * str);
size_t putc(char c);
size_t puti(int32_t num, uint8_t base, bool upper);
size_t putli(int64_t num, uint8_t base, bool upper);
size_t putu(uint32_t num, uint8_t base, bool upper);
size_t putlu(uint64_t num, uint8_t base, bool upper);

size_t printf(const char * fmt, ...);

size_t print_hexblock(const uint8_t * data, size_t count, size_t addr_offset);

#endif

#endif // LIBC_STDIO_H
