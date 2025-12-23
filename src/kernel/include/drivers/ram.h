#ifndef KERNEL_RAM_H
#define KERNEL_RAM_H

#include <stddef.h>
#include <stdint.h>

#include "defs.h"

#define REGION_TABLE_SIZE (PAGE_SIZE / sizeof(ram_table_entry_t)) // 512

typedef struct _ram_table_entry {
    uint32_t addr_flags;
    uint16_t page_count;
    uint16_t free_count;
} __attribute__((packed)) ram_table_entry_t;

typedef struct _ram_table {
    ram_table_entry_t entries[REGION_TABLE_SIZE];
} __attribute__((packed)) ram_table_t;

/**
 * @brief Initialize physical memory allocator.
 *
 * The bitmask vaddr must be a continuous set of pages equal to the number of
 * region tables * page size.
 *
 * @param ram_table pointer to the ram region table
 * @param bitmasks virtual address of the bitmasks
 * @return int 0 for success
 */
int ram_init(ram_table_t * ram_table, void * bitmasks);

/**
 * @brief Add a continuous, usable, physical memory address and length.
 *
 * This memory may be split into multiple regions if it is large enough to
 * justify the split. In this case ram_region_table_count will return more than
 * 1 per call to this function. There will be at least 1 per call to this
 * function.
 *
 * Paging / virtual memory MUST BE DISABLED to call this function. It will fail
 * if paging is enabled.
 *
 * `base` must be page aligned.
 *
 * @param base address of the memory
 * @param length size in bytes of the memory
 * @return int 0 for success
 */
int ram_region_add_memory(uint64_t base, uint64_t length);

/**
 * @brief Get the number of regions in the region table.
 *
 * @return size_t number of regions
 */
size_t ram_region_table_count();

/**
 * @brief Get a total number of available pages.
 *
 * @return size_t number of free pages
 */
size_t ram_free_pages();

/**
 * @brief Get a total number of pages.
 *
 * @return size_t total number of pages
 */
size_t ram_max_pages();

/**
 * @brief Allocate a single page and return it's physical address.
 *
 * The address will always be page aligned.
 *
 * Paging / virtual memory MUST BE ENABLED. If this function is called without
 * paging, bad things will happen. There is no check for paging enabled /
 * disabled state.
 *
 * @return uint32_t page addres or 0 for failure
 */
uint32_t ram_page_alloc();

/**
 * @brief Allocate a single page and return it's physical address.
 *
 * The address will always be page aligned.
 *
 * This function is the variant of `ram_page_alloc` for use before paging is
 * enabled. Once paging is enabled this function will not work.
 *
 * Paging / virtual memory MUST BE DISABLED to call this function. Bad things
 * will happen if paging is enabled. This function does have a check for paging
 * since it is not intended to be used after paging is enabled.
 *
 * @return uint32_t page address or 0 for failure
 */
uint32_t ram_page_palloc();

/**
 * @brief Free a page allocated by ram_page_alloc.
 *
 * Paging / virtual memory MUST BE ENABLED. If this function is called without
 * paging, bad things will happen. There is no check for paging enabled /
 * disabled state.
 *
 * @param addr physical address of the page
 * @return int 0 for success
 */
int ram_page_free(uint32_t addr);

#endif // KERNEL_RAM_H
