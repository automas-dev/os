#include "libc/memory_alloc.h"

#include "config.h"
#include "libc/proc.h"
#include "libc/string.h"

#define PAGE_SIZE 4096

#define MAGIC_FREE 0x46524545
#define MAGIC_USED 0x55534544

#define NOT_ALIGNED(SIZE)         ((uint32_t)(SIZE) & 0x3)
#define IS_ALIGNED(SIZE)          (!NOT_ALIGNED(SIZE))
#define SHOULD_SPLIT(ENTRY, SIZE) ((ENTRY)->size >= (SIZE) + sizeof(memory_entry_t) + 4)

#define ALIGN_SIZE(SIZE)                   \
    if ((SIZE) & 0x3) {                    \
        (SIZE) = (((SIZE) >> 2) + 1) << 2; \
    }

static void             memory_split_entry(memory_t * mem, memory_entry_t * entry, size_t size);
static void             memory_merge_with_next(memory_t * mem, memory_entry_t * entry);
static memory_entry_t * memory_find_entry_size(memory_t * mem, size_t size);
static memory_entry_t * memory_find_entry_ptr(memory_t * mem, void * ptr);
static memory_entry_t * memory_add_entry(memory_t * mem, size_t size);

#if KERNEL_MEMORY_INTEGRITY_CHECKS_ENABLED
#define CHECK_ENTRY(ENTRY) catch_invalid_entry(ENTRY)
static void catch_invalid_entry(const memory_entry_t * entry) {
    if (!entry) {
        PANIC("INVALID MEMORY ENTRY");
    }
    if (entry->magic != MAGIC_FREE && entry->magic != MAGIC_USED) {
        PANIC("INVALID MEMORY ENTRY");
    }
    if (entry->next) {
        if (entry->next->magic != MAGIC_FREE && entry->next->magic != MAGIC_USED) {
            PANIC("INVALID MEMORY ENTRY");
        }
    }
    if (entry->prev) {
        if (entry->prev->magic != MAGIC_FREE && entry->prev->magic != MAGIC_USED) {
            PANIC("INVALID MEMORY ENTRY");
        }
    }
}
#else
#define CHECK_ENTRY(ENTRY)
#endif

int memory_init(memory_t * mem, memory_alloc_pages_t alloc_pages_fn) {
    if (!mem) {
        return -1;
    }
    if (!alloc_pages_fn) {
        return -1;
    }

    mem->first          = alloc_pages_fn(1);
    mem->last           = mem->first;
    mem->alloc_pages_fn = alloc_pages_fn;

    if (!mem->first) {
        return -1;
    }

    memory_entry_t * entry = mem->first;

    entry->magic = MAGIC_FREE;
    entry->size  = PAGE_SIZE - sizeof(memory_entry_t);
    entry->next  = 0;
    entry->prev  = 0;

    CHECK_ENTRY(entry);

    return 0;
}

void * memory_alloc(memory_t * mem, size_t size) {
    if (!mem) {
        return 0;
    }
    if (!size) {
        return 0;
    }

    ALIGN_SIZE(size);

    memory_entry_t * entry = memory_find_entry_size(mem, size);

    if (!entry) {
        size_t request_size = size;

        // Last entry will be merged with new entry
        if (mem->last->magic == MAGIC_FREE) {
            request_size -= mem->last->size - sizeof(memory_entry_t);
        }

        entry = memory_add_entry(mem, request_size);
        CHECK_ENTRY(entry);

        if (!entry) {
            return 0;
        }

        // Last entry is being included, so merge it
        if (request_size != size) {
            entry = entry->prev;
            CHECK_ENTRY(entry);
            memory_merge_with_next(mem, entry);
        }
    }

    CHECK_ENTRY(entry);

    if (SHOULD_SPLIT(entry, size)) {
        memory_split_entry(mem, entry, size);
        CHECK_ENTRY(entry);
    }

    entry->magic = MAGIC_USED;
    CHECK_ENTRY(entry);

    return ENTRY_PTR(entry);
}

void * memory_realloc(memory_t * mem, void * ptr, size_t size) {
    if (!mem) {
        return 0;
    }
    if (!ptr) {
        return 0;
    }
    if (!size) {
        return 0;
    }

    // Will never be found
    if (NOT_ALIGNED(ptr)) {
        return 0;
    }

    ALIGN_SIZE(size);

    memory_entry_t * entry = memory_find_entry_ptr(mem, ptr);
    CHECK_ENTRY(entry);

    // Does not exist
    if (!entry) {
        return 0;
    }
    if (entry->magic != MAGIC_USED) {
        return 0;
    }

    // Same size or smaller
    if (entry->size <= size) {
        // TODO split / shrink entry
        return ENTRY_PTR(entry);
    }

    // TODO handle shrink
    // // Shrink
    // if (entry->size < size) {
    //     if (SHOULD_SPLIT(entry, size) && memory_split_entry(mem, entry, size)) {
    //         return 0;
    //     }

    //     return ENTRY_PTR(entry);
    // }

    void * new_ptr = memory_alloc(mem, size);
    if (!new_ptr) {
        return 0;
    }

    size_t min_size = size;
    if (entry->size < min_size) {
        min_size = entry->size;
    }

    kmemmove(new_ptr, ptr, min_size);

    memory_free(mem, ptr);

    return new_ptr;
}

int memory_free(memory_t * mem, void * ptr) {
    if (!mem) {
        return -1;
    }
    if (!ptr) {
        return -1;
    }

    // Will never be found
    if (NOT_ALIGNED(ptr)) {
        return -1;
    }

    memory_entry_t * entry = memory_find_entry_ptr(mem, ptr);
    CHECK_ENTRY(entry);

    // Does not exist
    if (!entry) {
        return -1;
    }
    if (entry->magic != MAGIC_USED) {
        return -1;
    }

    entry->magic = MAGIC_FREE;
    CHECK_ENTRY(entry);

    return 0;
}

/**
 * @brief Split a memory entry such that the first entry is at least `size`.
 *
 * If the entry cannot be split, this function will fail.
 *
 * @param mem pointer to the memory allocator
 * @param entry pointer to the memory entry
 * @param size minimum number of bytes
 */
static void memory_split_entry(memory_t * mem, memory_entry_t * entry, size_t size) {
    CHECK_ENTRY(entry);
    memory_entry_t * new_entry = ENTRY_PTR(entry) + size;

    new_entry->magic = MAGIC_FREE;
    new_entry->size  = entry->size - size - sizeof(memory_entry_t);
    new_entry->prev  = entry;
    new_entry->next  = entry->next;
    CHECK_ENTRY(new_entry);

    if (entry == mem->last) {
        mem->last = new_entry;
        CHECK_ENTRY(entry);
        CHECK_ENTRY(new_entry);
        CHECK_ENTRY(mem->last);
    }

    if (entry->next) {
        CHECK_ENTRY(entry->next);
        entry->next->prev = new_entry;
        CHECK_ENTRY(entry);
        CHECK_ENTRY(new_entry);
        CHECK_ENTRY(entry->next);
        CHECK_ENTRY(entry->next->prev);
    }

    entry->next = new_entry;
    entry->size = size;
    CHECK_ENTRY(entry);
    CHECK_ENTRY(entry->next);
}

/**
 * @brief Merge `entry` and the next.
 *
 * If the next entry is not free or there is no next entry, this function will
 * fail.
 *
 * @param mem pointer to the memory allocator
 * @param entry pointer to the memory entry
 */
static void memory_merge_with_next(memory_t * mem, memory_entry_t * entry) {
    CHECK_ENTRY(entry);
    memory_entry_t * next_entry = entry->next;
    CHECK_ENTRY(next_entry);

    if (next_entry == mem->last) {
        mem->last = entry;
        CHECK_ENTRY(mem->last);
    }

    entry->size += next_entry->size + sizeof(memory_entry_t);
    entry->next = next_entry->next;
    CHECK_ENTRY(entry);
    CHECK_ENTRY(next_entry);

    if (next_entry->next) {
        next_entry->next->prev = entry;
        CHECK_ENTRY(entry);
        CHECK_ENTRY(next_entry);
    }
}

/**
 * @brief Find a memory entry that is free and is at least `size` bytes.
 *
 * This function will join any adjacent free entries while searching. If the
 * memory entry is larger than size, it will not be split.
 *
 * @param mem pointer to the memory allocator
 * @param size minimum number of bytes
 * @return memory_entry_t* pointer to the memory entry
 */
static memory_entry_t * memory_find_entry_size(memory_t * mem, size_t size) {
    memory_entry_t * entry = mem->first;
    CHECK_ENTRY(entry);

    while (entry) {
        CHECK_ENTRY(entry);

        if (entry->magic == MAGIC_FREE) {
            memory_entry_t * next_entry = entry->next;

            while (entry->size < size && next_entry) {
                CHECK_ENTRY(next_entry);
                if (next_entry->magic != MAGIC_FREE) {
                    break;
                }

                memory_merge_with_next(mem, entry);
                CHECK_ENTRY(entry);

                if (entry->size >= size) {
                    break;
                }

                next_entry = entry->next;
            }

            if (entry->size >= size) {
                return entry;
            }
        }

        entry = entry->next;
    }

    return 0;
}

/**
 * @brief Find a memory entry from it's allocated pointer.
 *
 * `ptr` is the value returned by `memory_alloc`.
 *
 * @param mem pointer to the memory allocator
 * @param ptr pointer to the allocated memory
 * @return memory_entry_t* pointer to the memory entry
 */
static memory_entry_t * memory_find_entry_ptr(memory_t * mem, void * ptr) {
    memory_entry_t * entry = mem->first;

    while (entry) {
        CHECK_ENTRY(entry);

        if (ENTRY_PTR(entry) == ptr) {
            return entry;
        }

        entry = entry->next;
    }

    return 0;
}

/**
 * @brief Allocate new pages to create a new memory entry.
 *
 * @param mem pointer to the memory allocator
 * @param size minimum number of bytes
 * @return memory_entry_t* pointer to the new entry
 */
static memory_entry_t * memory_add_entry(memory_t * mem, size_t size) {
    size += sizeof(memory_entry_t);

    size_t pages = size >> 12;
    if (size & 0xfff) {
        pages++;
    }

    void * new_pages = mem->alloc_pages_fn(pages);
    if (!new_pages) {
        return 0;
    }

    memory_entry_t * entry = new_pages;

    entry->magic    = MAGIC_FREE;
    entry->size     = pages * PAGE_SIZE - sizeof(memory_entry_t);
    entry->prev     = mem->last;
    mem->last->next = entry;
    mem->last       = entry;
    CHECK_ENTRY(entry);

    return entry;
}
