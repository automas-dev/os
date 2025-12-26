/**
 * @brief Loader starts in raw memory before paging is enabled. After paging is
 * enabled, initialize the kernel then load and launch init program.
 *
 * Documentation moved to design/boot_stages.md
 */

#include <stdint.h>

#include "boot_params.h"
#include "cpu/gdt.h"
#include "cpu/mmu.h"
#include "defs.h"
#include "drivers/ram.h"
#include "drivers/serial.h"
#include "drivers/tar.h"
#include "drivers/vga.h"
#include "kernel.h"
#include "kernel/device/screen.h"
#include "kernel/device/serial.h"
#include "kernel/logs.h"
#include "kernel/memory.h"
#include "kernel/panic.h"
#include "libc/file.h"
#include "libc/string.h"
#include "process.h"
#include "process_manager.h"

#undef SERVICE
#define SERVICE "LOADER"

void kernel_init();

static void map_kernel_table(mmu_table_t * table);
static void id_map_range(mmu_table_t * table, size_t start, size_t end);
static void id_map_page(mmu_table_t * table, size_t page);

static process_t * load_init();

extern volatile int start_now;

void __start() {
    start_now = 1;
    // 1. Load VGA driver and clear screen
    vga_init(UINT2PTR(PADDR_VGA));

    // 2. Setup kernel logging (screen only)
    serial_init(SERIAL_PORT_COM1);
    _libc_config_file_write_call(device_serial_write_raw);

    kernel_log_init();
    kernel_log_set_level(KERNEL_LOG_LEVEL_DEBUG);
    // kernel_log_set_level(KERNEL_LOG_LEVEL_TRACE);
    // KLOG_DEBUG("vga init finished");
    KLOG_INFO("Loader Start");

    // 3. Initialize ram table (physical memory)
    void * ram_table = UINT2PTR(PADDR_RAM_TABLE);

    if (ram_init(ram_table, UINT2PTR(VADDR_RAM_BITMASKS))) {
        KPANIC("Failed to initialize RAM");
    }

    KLOG_DEBUG("ram table created");

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

    KLOG_DEBUG("ram table init finished");

    // 4. Initialize kernel virtual memory
    // 4.1 Create page dir
    mmu_dir_t * pdir = UINT2PTR(PADDR_KERNEL_DIR);
    mmu_dir_clear(pdir);

    // This needs to be disabled here because something around enable paging blocks if it's enabled
    start_now = 0;

    KLOG_DEBUG("page dir created");

    // 4.2 Create first page table
    uint32_t first_table_addr = ram_page_palloc();
    mmu_dir_set(pdir, 0, first_table_addr, MMU_DIR_RW);

    KLOG_DEBUG("page table created");

    // 4.3 Map first page table
    mmu_table_t * first_table = UINT2PTR(first_table_addr);
    mmu_table_clear(first_table);
    map_kernel_table(first_table);

    // 4.4 Map last table to dir for access to tables
    mmu_dir_set(pdir, MMU_DIR_SIZE - 1, PADDR_KERNEL_DIR, MMU_DIR_RW);

    KLOG_DEBUG("kernel page table finished");

    // 5. Initialize GDT
    init_gdt();
    KLOG_DEBUG("gdt init finished");

    // 6. Initialize TSS
    init_tss();
    KLOG_DEBUG("tss init finished");

    // 7. Enable paging
    mmu_enable_paging(PADDR_KERNEL_DIR);
    // This needs to be enabled here because something around enable paging blocks if it's enabled
    start_now = 1;
    KLOG_DEBUG("paging enabled");

    // 8. Initialize kernel
    kernel_init();
    KLOG_DEBUG("kernel init finished");

    // 9. Load init executable
    process_t * init = load_init();
    if (!init) {
        KPANIC("Failed to load init executable");
    }
    KLOG_DEBUG("load init finished");

    // 10. Launch init (os main function)
    start_first_task(init);
    KLOG_WARNING("Returned from init");

    KLOG_INFO("Halting");
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

static char * copy_string(const char * str) {
    int    len     = kstrlen(str);
    char * new_str = kmalloc(len + 1);
    kmemcpy(new_str, str, len + 1);
    return new_str;
}

static int copy_args(process_t * proc, const char * filepath, int argc, const char ** argv) {
    if (!proc || !filepath || !argv) {
        return -1;
    }

    proc->filepath = copy_string(filepath);
    proc->argc     = argc;
    proc->argv     = kmalloc(sizeof(char *) * argc);
    for (int i = 0; i < argc; i++) {
        proc->argv[i] = copy_string(argv[i]);
    }

    return 0;
}

typedef int (*ff_t)(size_t argc, char ** argv);

static void proc_entry() {
    process_t * proc = get_active_task();
    ff_t        fn   = UINT2PTR(VADDR_USER_MEM);

    // KLOG_INFO("Start task %s with %u args", proc->filepath, proc->argc);

    // TODO get start function pointer from elf

    int res           = fn(proc->argc, proc->argv);
    proc->status_code = res;
}

static process_t * load_init() {
    const char * filename = "init";

    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        KLOGS_ERROR("init", "Failed to find file\n");
        return 0;
    }

    uint8_t * buff = kmalloc(stat.size);
    if (!buff) {
        return 0;
    }

    tar_fs_file_t * file = tar_file_open(kernel_get_tar(), filename);
    if (!file) {
        kfree(buff);
        return 0;
    }

    if (!tar_file_read(file, buff, stat.size)) {
        tar_file_close(file);
        kfree(buff);
        return 0;
    }

    process_t * proc = kmalloc(sizeof(process_t));

    if (process_create(proc)) {
        KLOGS_ERROR("init", "Failed to create process\n");
        return 0;
    }

    if (process_load_heap(proc, buff, stat.size)) {
        KLOGS_ERROR("init", "Failed to load\n");
        process_free(proc);
        return 0;
    }

    for (size_t i = 0; i < 1022; i++) {
        process_grow_stack(proc);
    }

    copy_args(proc, filename, 1, &filename);

    process_set_entrypoint(proc, proc_entry);
    process_add_pages(proc, 32);
    pm_add_proc(&get_kernel()->pm, proc);
    pm_set_foreground_proc(&get_kernel()->pm, proc->pid);

    tar_file_close(file);
    kfree(buff);

    return proc;
}
