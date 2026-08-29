#define KLOG_SERVICE "PAGING"

#include "paging.h"

#include "drivers/ram.h"
#include "kernel/logs.h"
#include "libc/string.h"

typedef struct {
    uint32_t addr;
    size_t   count;
} page_user_t;

static page_user_t __temp_pages[VADDR_TMP_PAGE_COUNT];

int paging_init() {
    if (!kmemset(__temp_pages, 0, sizeof(__temp_pages))) {
        KLOG_ERROR("Failed to clear temp pages array");
        return -1;
    }

    KLOG_DEBUG("Paging initialized");
    return 0;
}

void * paging_temp_map(uint32_t paddr) {
    if (!paddr) {
        KLOG_WARNING("Trying to map physical address 0");
        return 0;
    }
    if (paddr & MASK_FLAGS) {
        KLOG_WARNING("Trying to map misaligned physical address %p", paddr);
        return 0;
    }

    KLOG_TRACE("Map temporary page for physical address %p", paddr);

    // Return one if already exists
    for (size_t i = 0; i < VADDR_TMP_PAGE_COUNT; i++) {
        if (__temp_pages[i].addr == paddr) {
            __temp_pages[i].count++;
            size_t table_i = ADDR2PAGE(VADDR_TMP_PAGE) + i;
            return UINT2PTR(PAGE2ADDR(table_i));
        }
    }

    // Find a free temp page to use
    for (size_t i = 0; i < VADDR_TMP_PAGE_COUNT; i++) {
        if (__temp_pages[i].count < 1) {
            __temp_pages[i].addr  = paddr;
            __temp_pages[i].count = 1;

            size_t table_i = ADDR2PAGE(VADDR_TMP_PAGE) + i;

            mmu_table_t * table = (mmu_table_t *)VADDR_KERNEL_TABLE;
            uint32_t      vaddr = PAGE2ADDR(table_i);

            KLOG_TRACE("Mapping physical address %p to table index %u virtual address %p", paddr, table_i, vaddr);

            if (mmu_table_set(table, table_i, paddr, MMU_TABLE_RW)) {
                KLOG_ERROR("Failed to set mmu table index %u to physical address %p", table_i, paddr);
                return 0;
            }
            mmu_flush_tlb(vaddr);
            return UINT2PTR(vaddr);
        }
    }

    return 0;
}

int paging_temp_free(uint32_t paddr) {
    if (!paddr) {
        KLOG_WARNING("Trying to free physical address 0");
        return -1;
    }
    if (paddr & MASK_FLAGS) {
        KLOG_WARNING("Trying to free misaligned physical address %p", paddr);
        return -1;
    }

    KLOG_TRACE("Free temporary page for physical address %p", paddr);

    for (size_t i = 0; i < VADDR_TMP_PAGE_COUNT; i++) {
        if (__temp_pages[i].addr == paddr) {
            if (!__temp_pages[i].count) {
                KLOG_WARNING("Trying to free temp page %u which has count 0", i);
                return -1;
            }

            __temp_pages[i].count--;

            uint32_t vaddr = PAGE2ADDR(i);
            KLOG_TRACE("Freed temporary page %u with virtual address %p from physical address %p", i, vaddr, paddr);

            return 0;
        }
    }

    KLOG_WARNING("Failed to find mapping for physical address %u", paddr);

    return -1;
}

size_t paging_temp_available() {
    size_t free = 0;

    for (size_t i = 0; i < VADDR_TMP_PAGE_COUNT; i++) {
        if (!__temp_pages[i].count) {
            free++;
        }
    }

    return free;
}

int paging_id_map_range(size_t start, size_t end) {
    if (start > end) {
        KLOG_WARNING("Trying to map range with start %u after end %u", start, end);
        return -1;
    }

    while (start <= end) {
        if (paging_id_map_page(start++)) {
            // Error logged in paging_id_map_page
            return -1;
        }
    }

    return 0;
}

int paging_id_map_page(size_t page) {
    if (page >= MMU_TABLE_SIZE) {
        KLOG_WARNING("Failed to identity map page %u", page);
        return -1;
    }

    mmu_table_t * table = (mmu_table_t *)VADDR_KERNEL_TABLE;
    mmu_table_set(table, page, page << 12, MMU_TABLE_RW);

    return 0;
}

int paging_add_pages(mmu_dir_t * dir, size_t start, size_t end) {
    if (!dir) {
        KLOG_WARNING("Trying to add page to null directory");
        return -1;
    }
    if (start > end) {
        KLOG_WARNING("Trying to add pages for range with start %u after end %u", start, end);
        return -1;
    }

    uint32_t table_end = end / MMU_TABLE_SIZE;

    if (table_end >= MMU_DIR_SIZE) {
        KLOG_WARNING("End %u is after last table end %u", end, MMU_DIR_SIZE * MMU_TABLE_SIZE);
        return -1;
    }

    // Add pages to tables
    for (size_t page_i = start; page_i <= end; page_i++) {
        uint32_t addr = ram_page_alloc();

        if (!addr) {
            if (paging_remove_pages(dir, start, page_i - 1)) {
                KLOG_DEBUG("Failed to remove page from directory");
            }
            return -1;
        }

        uint32_t dir_i   = page_i / MMU_TABLE_SIZE;
        uint32_t table_i = page_i % MMU_TABLE_SIZE;

        // Add table if needed
        if (!(mmu_dir_get_flags(dir, dir_i) & MMU_DIR_FLAG_PRESENT)) {
            if (paging_add_table(dir, dir_i)) {
                if (paging_remove_pages(dir, start, page_i - 1)) {
                    KLOG_DEBUG("Failed to remove page from directory %p", dir);
                }
                if (ram_page_free(addr)) {
                    KLOG_DEBUG("Failed to free ram page");
                }
                return -1;
            }
        }

        // Table will be present after previous step
        uint32_t      table_addr = mmu_dir_get_addr(dir, dir_i);
        mmu_table_t * table      = paging_temp_map(table_addr);

        if (!table) {
            KLOG_ERROR("Failed to create temporary map for %p", table_addr);
            if (paging_remove_pages(dir, start, page_i - 1)) {
                KLOG_DEBUG("Failed to remove page from directory %p while unwinding failed temp map", dir);
            }
            if (ram_page_free(addr)) {
                KLOG_DEBUG("Failed to free ram page while unwinding failed temp map");
            }
            return -1;
        }

        // TODO should either be panic or log error and return?
        if (mmu_table_set(table, table_i, addr, MMU_TABLE_RW)) {
            KLOG_ERROR("Failed to set page table %p index %u to physical address %p", table, table_i, addr);
            return -1;
        }
        if (paging_temp_free(table_addr)) {
            KLOG_DEBUG("Failed to free temporary page after setting page table");
            return -1;
        }
    }

    return 0;
}

int paging_remove_pages(mmu_dir_t * dir, size_t start, size_t end) {
    if (!dir) {
        KLOG_WARNING("Trying to remove page from null directory");
        return -1;
    }
    if (start > end) {
        KLOG_WARNING("Trying to remove pages for range with start %u after end %u", start, end);
        return -1;
    }

    uint32_t table_end = end / MMU_TABLE_SIZE;

    if (table_end >= MMU_DIR_SIZE) {
        KLOG_WARNING("End %u is after last table end %u", end, MMU_TABLE_SIZE);
        return -1;
    }

    // Remove pages from tables
    for (size_t page_i = start; page_i <= end; page_i++) {
        uint32_t dir_i   = page_i / MMU_TABLE_SIZE;
        uint32_t table_i = page_i % MMU_TABLE_SIZE;

        // Table is not present
        if (!(mmu_dir_get_flags(dir, dir_i) & MMU_DIR_FLAG_PRESENT)) {
            continue;
        }

        // Table will be present after previous step
        uint32_t      table_addr = mmu_dir_get_addr(dir, dir_i);
        mmu_table_t * table      = paging_temp_map(table_addr);

        if (!table) {
            KLOG_ERROR("Failed to create temporary map for %p", table_addr);
            return -1;
        }

        if (!(mmu_table_get_flags(table, table_i) & MMU_TABLE_FLAG_PRESENT)) {
            paging_temp_free(table_addr);
            continue;
        }

        uint32_t page_addr = mmu_table_get_addr(table, table_i);

        // TODO should either be panic or log error and return?
        if (mmu_table_set(table, table_i, 0, 0)) {
            KLOG_ERROR("Failed to clear page table %p index %u", table, table_i);
            return -1;
        }
        mmu_flush_tlb(PAGE2ADDR(page_i));
        if (ram_page_free(page_addr)) {
            KLOG_DEBUG("Failed to free ram page while removing pages");
            return -1;
        }
        if (paging_temp_free(table_addr)) {
            KLOG_DEBUG("Failed to free temporary mapping for table %p while removing pages", table_addr);
            return -1;
        }
    }

    return 0;
}

int paging_add_table(mmu_dir_t * dir, size_t dir_i) {
    if (!dir) {
        KLOG_WARNING("Trying to add table to null directory");
        return -1;
    }
    if (dir_i >= MMU_DIR_SIZE) {
        KLOG_WARNING("Directory index %u is past directory end %p", dir_i, MMU_DIR_SIZE);
        return -1;
    }

    if (!(mmu_dir_get_flags(dir, dir_i) & MMU_DIR_FLAG_PRESENT)) {
        uint32_t addr = ram_page_alloc();

        if (!addr) {
            return -1;
        }

        mmu_table_t * table = paging_temp_map(addr);

        if (!table) {
            if (ram_page_free(addr)) {
                KLOG_DEBUG("Failed to free ram page");
            }
            return -1;
        }

        mmu_table_clear(table);
        if (mmu_dir_set(dir, dir_i, addr, MMU_DIR_RW)) {
            KLOG_ERROR("Failed to set page directory %p index %u to physical address %p", dir, dir_i, addr);
            if (paging_temp_free(addr)) {
                KLOG_DEBUG("Failed to free temporary page");
            }
            return -1;
        }

        paging_temp_free(addr);
    }

    return 0;
}

int paging_remove_table(mmu_dir_t * dir, size_t dir_i) {
    if (!dir) {
        KLOG_WARNING("Trying to remove table from null directory");
        return -1;
    }
    if (dir_i >= MMU_DIR_SIZE) {
        KLOG_WARNING("Directory index %u is past directory end %p", dir_i, MMU_DIR_SIZE);
        return -1;
    }

    if (mmu_dir_get_flags(dir, dir_i) & MMU_DIR_FLAG_PRESENT) {
        uint32_t addr = mmu_dir_get_addr(dir, dir_i);
        if (!addr) {
            KLOG_ERROR("Failed to get address of table %u from directory %p", dir_i, dir);
            return -1;
        }
        if (mmu_dir_set(dir, dir_i, 0, 0)) {
            KLOG_ERROR("Failed to clear table %u from directory %p", dir_i, dir);
            return -1;
        }
        if (ram_page_free(addr)) {
            KLOG_DEBUG("Failed to free ram page while removing table");
        }
    }

    return 0;
}
