#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

#include "addr.h"
#include "cpu/mmu.h"

/**
 * @brief Setup paging called by kernel main.
 * @return int 0 for success
 */
int paging_init();

/**
 * @brief Map `paddr` to a temporary page and return the virtual address.
 *
 * If `paddr` has already been allocated, return that address and increment the
 * use counter.
 *
 * @param paddr physical address to map
 * @return void* pointer to the virtual page address
 */
void * paging_temp_map(uint32_t paddr);

/**
 * @brief Free `paddr` previously mapped with `paging_temp_map`.
 *
 * The `paddr` argument should be the same as passed to `paging_temp_map`. If
 * `paddr` has been mapped multiple times, the use counter will be decremented
 * until empty.
 *
 * @param paddr physical address to free
 * @return int 0 for success
 */
int paging_temp_free(uint32_t paddr);

/**
 * @brief Get count of free pages for temporary mapping.
 *
 * @return size_t number of free temporary pages
 */
size_t paging_temp_available();

/**
 * @brief Identity map a range of pages.
 *
 * The range is end inclusive, ie a page will be added for end.
 *
 * start and end must be < MMU_TABLE_SIZE (ie. only the firs table can be
 * identity mapped).
 *
 * @param start first page index
 * @param end last page index (inclusive)
 * @return int 0 for success
 */
int paging_id_map_range(size_t start, size_t end);

/**
 * @brief Identity map a single page.
 *
 * page must be < MMU_TABLE_SIZE (ie. only the firs table can be identity
 * mapped).
 *
 * @param page page index
 * @return int 0 for success
 */
int paging_id_map_page(size_t page);

/**
 * @brief Allocate physical memory and map it to a range of pages.
 *
 * The range is end inclusive, ie a page will be added for end.
 *
 * @param dir pointer to the page directory
 * @param start first page index
 * @param end last page index (inclusive)
 * @return int 0 for success
 */
int paging_add_pages(mmu_dir_t * dir, size_t start, size_t end);

/**
 * @brief Free physical memory for a range of pages.
 *
 * The range is end inclusive, ie a page will be freed for end.
 *
 * This function does not free the page tables, it only frees their pages.
 *
 * @param dir pointer to the page directory
 * @param start first page index
 * @param end last page index (inclusive)
 * @return int 0 for success
 */
int paging_remove_pages(mmu_dir_t * dir, size_t start, size_t end);

/**
 * @brief Add a table to the current page directory.
 *
 * @param dir pointer to the page directory
 * @param dir_i table index in page directory
 * @return int 0 for success
 */
int paging_add_table(mmu_dir_t * dir, size_t dir_i);

/**
 * @brief Remove a table from the current page directory.
 *
 * @param dir pointer to the page directory
 * @param dir_i table index in page directory
 * @return int 0 for success
 */
int paging_remove_table(mmu_dir_t * dir, size_t dir_i);

#endif // PAGING_H
