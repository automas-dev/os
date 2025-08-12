#include "libc/memory.h"

#include "libk/sys_call.h"

_libc_config_malloc_call_fn  __malloc_call  = _sys_mem_malloc;
_libc_config_realloc_call_fn __realloc_call = _sys_mem_realloc;
_libc_config_free_call_fn    __free_call    = _sys_mem_free;

void * pmalloc(size_t size) {
    return __malloc_call(size);
}

void * prealloc(void * ptr, size_t size) {
    return __realloc_call(ptr, size);
}

void pfree(void * ptr) {
    __free_call(ptr);
}

void _libc_config_malloc_call(_libc_config_malloc_call_fn fn) {
    __malloc_call = fn;
}

void _libc_config_realloc_call(_libc_config_realloc_call_fn fn) {
    __realloc_call = fn;
}

void _libc_config_free_call(_libc_config_free_call_fn fn) {
    __free_call = fn;
}
