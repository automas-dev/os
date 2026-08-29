#ifndef MEMORY_ALLOC_H
#define MEMORY_ALLOC_H

#include <stddef.h>
#include <stdint.h>

#define ENTRY_PTR(ENTRY) ((void *)((uint32_t)(ENTRY) + sizeof(memory_entry_t)))

typedef void * (*memory_alloc_pages_t)(size_t pages);

typedef struct _entry {
    uint32_t        magic;
    struct _entry * next;
    struct _entry * prev;
    size_t          size;
} __attribute__((packed)) memory_entry_t;

typedef struct _memory {
    memory_entry_t *     first;
    memory_entry_t *     last;
    memory_alloc_pages_t alloc_pages_fn;
} memory_t;

/**
 * @brief Setup a new memory allocator.
 *
 * This will call `alloc_page_fn` once to setup the first entry.
 *
 * @param mem pointer to a memory allocator
 * @param alloc_pages_fn callback to allocate more pages
 * @return int 0 for success
 */
int memory_init(memory_t * mem, memory_alloc_pages_t alloc_pages_fn);

/**
 * @brief Allocate a region of memory and return it's pointer.
 *
 * ``size`` must not be 0. If size is not aligned to 4 bytes, it will be rounded
 * up to the next alignment boundary. The pointer returned will always be
 * aligned to 4 bytes.
 *
 * @param mem pointer to the memory allocator
 * @param size minimum number of bytes to allocate
 * @return void* pointer to the allocated memory or 0 for fail
 */
void * memory_alloc(memory_t * mem, size_t size);

/**
 * @brief NOT IMPLEMENTED
 */
void * memory_realloc(memory_t * mem, void * ptr, size_t size);

/**
 * @brief Free allocated memory.
 *
 * @param mem pointer to the memory allocator
 * @param ptr pointer to the allocated memory
 * @return int 0 for success
 */
int memory_free(memory_t * mem, void * ptr);

#endif // MEMORY_ALLOC_H
