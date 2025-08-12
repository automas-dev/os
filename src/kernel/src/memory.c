#include "kernel/memory.h"

#include "kernel.h"
#include "kernel/logs.h"
#include "libc/memory.h"
#include "paging.h"

static memory_t __memory;
static size_t   __next_heap_page;

void init_kmalloc(size_t next_heap_page) {
    KLOGS_DEBUG("kmalloc", "Init kmalloc to heap page 0x%X", next_heap_page);

    __next_heap_page = next_heap_page;
    memory_init(&__memory, kernel_alloc_page);

    KLOGS_DEBUG("kmalloc", "Override libc pmalloc functions");
    _libc_config_malloc_call(kmalloc);
    _libc_config_realloc_call(krealloc);
    _libc_config_free_call(kfree);
}

void * kmalloc(size_t size) {
    return memory_alloc(&__memory, size);
}

void * krealloc(void * ptr, size_t size) {
    return memory_realloc(&__memory, ptr, size);
}

void kfree(void * ptr) {
    memory_free(&__memory, ptr);
}

void * kernel_alloc_page(size_t count) {
    if (!count) {
        return 0;
    }

    if (__next_heap_page + count >= MMU_DIR_SIZE * MMU_TABLE_SIZE) {
        return 0;
    }

    mmu_dir_t * dir = paging_temp_map(get_kernel()->cr3);

    if (!dir) {
        return 0;
    }

    if (paging_add_pages(dir, __next_heap_page, __next_heap_page + count)) {
        paging_temp_free(get_kernel()->cr3);
        return 0;
    }

    paging_temp_free(get_kernel()->cr3);

    void * ptr = UINT2PTR(PAGE2ADDR(__next_heap_page));
    __next_heap_page += count;

    return ptr;
}
