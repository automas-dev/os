#ifndef LIBC_STDIO_IMPL_H
#define LIBC_STDIO_IMPL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef size_t (*puts_fn)(const char * s);
typedef size_t (*putc_fn)(char c);

size_t vputi(puts_fn ps, putc_fn pc, int32_t num, uint8_t base, bool upper);
size_t vputli(puts_fn ps, putc_fn pc, int64_t num, uint8_t base, bool upper);
size_t vputu(puts_fn ps, putc_fn pc, uint32_t num, uint8_t base, bool upper);
size_t vputlu(puts_fn ps, putc_fn pc, uint64_t num, uint8_t base, bool upper);

size_t vprintf(puts_fn ps, putc_fn pc, const char * fmt, ...);

#endif // LIBC_STDIO_IMPL_H
