#include "libc/memory.h"

#include "libc/memory_alloc.h"
#include "libk/sys_call.h"

// Default backing allocator for pmalloc/prealloc/pfree: a heap allocator
// (src/libc/src/memory_alloc.c) running entirely in this process' own
// address space, only reaching into the kernel (via _sys_mem_alloc_page) to
// map in more heap pages when it actually runs out of room. This is lazily
// initialized on first use rather than eagerly, since memory_init needs to
// perform its own first page allocation, which in turn requires the syscall
// to actually be usable (ie. this process must be running).
static memory_t __memory;
static int      __memory_ready = 0;

static void * heap_alloc_page(size_t count) {
    return _sys_mem_alloc_page(count);
}

static int ensure_memory_ready(void) {
    if (__memory_ready) {
        return 0;
    }

    if (memory_init(&__memory, heap_alloc_page)) {
        return -1;
    }

    __memory_ready = 1;

    return 0;
}

static void * default_malloc(size_t size) {
    if (ensure_memory_ready()) {
        return 0;
    }

    return memory_alloc(&__memory, size);
}

static void * default_realloc(void * ptr, size_t size) {
    if (ensure_memory_ready()) {
        return 0;
    }

    return memory_realloc(&__memory, ptr, size);
}

static void default_free(void * ptr) {
    if (ensure_memory_ready()) {
        return;
    }

    memory_free(&__memory, ptr);
}

static _libc_config_malloc_call_fn  __malloc_call  = default_malloc;
static _libc_config_realloc_call_fn __realloc_call = default_realloc;
static _libc_config_free_call_fn    __free_call    = default_free;

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
