#ifndef LIBC_MEMORY_H
#define LIBC_MEMORY_H

#include <stddef.h>
#include <stdint.h>

void * pmalloc(size_t size);
void * prealloc(void * ptr, size_t size);
void   pfree(void * ptr);

typedef void * (*_libc_config_malloc_call_fn)(size_t);
typedef void * (*_libc_config_realloc_call_fn)(void *, size_t);
typedef void (*_libc_config_free_call_fn)(void *);

void _libc_config_malloc_call(_libc_config_malloc_call_fn fn);
void _libc_config_realloc_call(_libc_config_realloc_call_fn fn);
void _libc_config_free_call(_libc_config_free_call_fn fn);

#endif // LIBC_MEMORY_H
