/**
 * @brief Lower kernel operates in raw memory before paging is enabled.
 *
 * 1. Clear screen
 * 2. Initialize ram table (physical memory)
 * 3. Setup paging (virtual memory)
 * 4. Map kernel table
 * 5. Jump to upper kernel (`kernel_main`)
 *
 * The upper kernel `kernel_main` is launched once paging has been enabled.
 */

#include <stdint.h>

#include "boot_params.h"
#include "cpu/gdt.h"
#include "cpu/mmu.h"
#include "defs.h"
#include "kernel/device/ram.h"
#include "kernel/device/screen.h"
#include "kernel/logs.h"
#include "kernel/panic.h"
#include "libc/file.h"
#include "vga.h"

void kernel_main();

static void map_kernel_table(mmu_table_t * table);
static void id_map_range(mmu_table_t * table, size_t start, size_t end);
static void id_map_page(mmu_table_t * table, size_t page);

void __start() {
    vga_init();

    _libc_config_file_write_call(device_screen_write_raw);

    kernel_log_set_level(KERNEL_LOG_LEVEL_DEBUG);
    KLOGS_DEBUG("loader", "vga init finished");
    KLOGS_INFO("loader", "Loader Start");

    void * ram_table = UINT2PTR(PADDR_RAM_TABLE);

    if (ram_init(ram_table, UINT2PTR(VADDR_RAM_BITMASKS))) {
        KPANIC("Failed to initialize RAM");
    }

    KLOGS_DEBUG("loader", "ram table created");

    boot_params_t * bparams = get_boot_params();

    for (size_t i = 0; i < bparams->mem_entries_count; i++) {
        upper_ram_t * entry = &bparams->mem_entries[i];

        // End of second stage kernel
        if (entry->base_addr <= 0x9fbff) {
            continue;
        }

        if (entry->type == RAM_TYPE_USABLE || entry->type == RAM_TYPE_ACPI_RECLAIMABLE) {
            ram_region_add_memory(entry->base_addr, entry->length);
        }
    }

    KLOGS_DEBUG("loader", "ram table init finished");

    mmu_dir_t * pdir = UINT2PTR(PADDR_KERNEL_DIR);
    mmu_dir_clear(pdir);

    KLOGS_DEBUG("loader", "page dir created");

    // Init first table
    uint32_t first_table_addr = ram_page_palloc();
    mmu_dir_set(pdir, 0, first_table_addr, MMU_DIR_RW);

    KLOGS_DEBUG("loader", "page table created");

    // Map first table
    mmu_table_t * first_table = UINT2PTR(first_table_addr);
    mmu_table_clear(first_table);
    map_kernel_table(first_table);

    // Map last table to dir for access to tables
    mmu_dir_set(pdir, MMU_DIR_SIZE - 1, PADDR_KERNEL_DIR, MMU_DIR_RW);

    KLOGS_DEBUG("loader", "kernel page table finished");

    init_gdt();
    KLOGS_DEBUG("loader", "gdt init finished");

    init_tss();
    KLOGS_DEBUG("loader", "tss init finished");

    mmu_enable_paging(PADDR_KERNEL_DIR);
    KLOGS_DEBUG("loader", "paging enabled");

    kernel_main();

    KLOGS_INFO("loader", "Halting");
    halt();
}

static void map_kernel_table(mmu_table_t * table) {
    // null page 0
    mmu_table_set(table, 0, 0, 0);

    // Page Directory
    mmu_table_set(table, 1, PADDR_KERNEL_DIR, MMU_DIR_RW);

    // Create first table
    mmu_table_set(table, 2, PADDR_RAM_TABLE, MMU_DIR_RW);

    // Stack
    id_map_range(table, 3, 6);

    // Kernel
    id_map_range(table, 7, 0x9e);

    // VGA
    id_map_page(table, 0xb8);

    // Kernel Table
    mmu_table_set(table, ADDR2PAGE(VADDR_KERNEL_TABLE), (uint32_t)table, MMU_TABLE_RW);

    // RAM region bitmasks
    ram_table_t * ram_table = UINT2PTR(PADDR_RAM_TABLE);

    for (size_t i = 0; i < ram_region_table_count(); i++) {
        uint32_t bitmask_addr = ram_table->entries[i].addr_flags & MASK_ADDR;
        mmu_table_set(table, ADDR2PAGE(VADDR_RAM_BITMASKS) + i, bitmask_addr, MMU_TABLE_RW);
    }
}

static void id_map_range(mmu_table_t * table, size_t start, size_t end) {
    if (end > 1023) {
        KPANIC("End is past table limits");
    }

    while (start <= end) {
        id_map_page(table, start);
        start++;
    }
}

static void id_map_page(mmu_table_t * table, size_t page) {
    mmu_table_set(table, page, page << 12, MMU_TABLE_RW);
}
