#ifndef LIBC_STDIO_IMPL_H
#define LIBC_STDIO_IMPL_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "libc/file.h"

size_t vputi(file_t * file, int32_t num, uint8_t base, bool upper);
size_t vputli(file_t * file, int64_t num, uint8_t base, bool upper);
size_t vputu(file_t * file, uint32_t num, uint8_t base, bool upper);
size_t vputlu(file_t * file, uint64_t num, uint8_t base, bool upper);
size_t vputs(file_t * file, const char * str);

size_t vaprintf(file_t * file, const char * fmt, ...);
size_t vprintf(file_t * file, const char * fmt, va_list params);

#endif // LIBC_STDIO_IMPL_H
