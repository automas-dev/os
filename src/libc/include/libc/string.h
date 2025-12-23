#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int    kmemcmp(const void * lhs, const void * rhs, size_t n);
void * kmemcpy(void * dest, const void * src, size_t n);
void * kmemmove(void * dest, const void * src, size_t n);
void * kmemset(void * dest, int value, size_t n);

size_t kstrlen(const char * str);
size_t knstrlen(const char * str, int max);
int    kstrcmp(const char * lhs, const char * rhs);
char * kstrfind(const char * str, int c);
// char * kstrtok(char * str, const char * delim);

int katoi(const char * str);

size_t itoa(int32_t n, char * str);
size_t itoa_base(size_t max_length, int32_t n, char * str, uint8_t base, bool upper);
size_t ltoa(int64_t n, char * str);
size_t ltoa_base(size_t max_length, int64_t n, char * str, uint8_t base, bool upper);

size_t utoa(uint32_t n, char * str);
size_t utoa_base(size_t max_length, uint32_t n, char * str, uint8_t base, bool upper);
size_t ultoa(uint64_t n, char * str);
size_t ultoa_base(size_t max_length, uint64_t n, char * str, uint8_t base, bool upper);

#endif // LIBC_STRING_H
