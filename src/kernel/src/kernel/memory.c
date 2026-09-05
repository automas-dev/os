#define KLOG_SERVICE "KERNEL/MEMORY"

#include "kernel/memory.h"

#include "kernel.h"
#include "kernel/logs.h"
#include "libc/memory.h"
#include "paging.h"

static memory_t __memory;
static size_t   __next_heap_page;

void kmalloc_init(size_t next_heap_page) {
    KLOG_DEBUG("Init kmalloc to heap page 0x%X", next_heap_page);

    __next_heap_page = next_heap_page;
    memory_init(&__memory, kernel_alloc_page);

    KLOG_TRACE("Override libc pmalloc functions");
    _libc_config_malloc_call(kmalloc);
    _libc_config_realloc_call(krealloc);
    _libc_config_free_call(kfree);

    KLOG_DEBUG("Initialized kernel memory management");
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
        KLOG_WARNING("Tried to allocate 0 pages");
        return 0;
    }

    if (__next_heap_page + count >= MMU_DIR_SIZE * MMU_TABLE_SIZE) {
        KLOG_WARNING("Tried to allocate past 4 GB");
        return 0;
    }

    mmu_dir_t * dir = paging_temp_map(VADDR_KERNEL_DIR);

    if (!dir) {
        KLOG_ERROR("Could not create temporary map for mmu dir");
        return 0;
    }

    if (paging_add_pages(dir, __next_heap_page, __next_heap_page + count, MMU_TABLE_RW)) {
        KLOG_ERROR("Failed to add %u pages starting at %u", count, __next_heap_page);
        paging_temp_free(VADDR_KERNEL_DIR);
        return 0;
    }

    paging_temp_free(VADDR_KERNEL_DIR);

    void * ptr = UINT2PTR(PAGE2ADDR(__next_heap_page));
    __next_heap_page += count;

    return ptr;
}
