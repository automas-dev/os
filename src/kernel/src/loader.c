/**
 * @brief Loader starts in raw memory before paging is enabled. After paging is
 * enabled, initialize the kernel then load and launch init program.
 *
 * Documentation moved to design/boot_stages.md
 */
#define KLOG_SERVICE "LOADER"

#include <stdint.h>

#include "boot_params.h"
#include "config.h"
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

void kernel_init();

static void map_kernel_table(mmu_table_t * table);
static void id_map_range(mmu_table_t * table, size_t start, size_t end);
static void id_map_page(mmu_table_t * table, size_t page);

static process_t * load_init();

extern volatile int start_now;

void __start() {
    start_now = 1;

    // 1. Setup kernel logging (serial only)
    kernel_log_init();
    serial_init(SERIAL_PORT_COM1);
    _libc_config_file_write_call(device_serial_write_raw);

    // THIS WAS REPLACED WITH KLOG_LEVEL MACRO
    // kernel_log_set_level(KERNEL_LOG_LEVEL_DEBUG);
    // kernel_log_set_level(KERNEL_LOG_LEVEL_TRACE);
    KLOG_INFO("Loader Start");

    KLOG_INFO(PROJECT_DESCRIPTION);

    // 2. Load VGA driver and clear screen
    vga_init(UINT2PTR(PADDR_VGA));
    KLOG_DEBUG("vga init finished");

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
    if (!str) {
        KLOG_ERROR("Tried to copy null string");
        return 0;
    }
    int    len     = kstrlen(str);
    char * new_str = kmalloc(len + 1);
    if (!new_str) {
        KLOG_ERROR("Failed to malloc new string of length %d", len + 1);
        return 0;
    }
    if (!kmemcpy(new_str, str, len + 1)) {
        KLOG_ERROR("Failed to copy %u bytes in memory from %p to %p", len + 1, str, new_str);
        return 0;
    }
    return new_str;
}

static int copy_args(process_t * proc, const char * filepath, int argc, char ** argv) {
    if (!proc) {
        KLOG_ERROR("Tried to copy args for null process");
        return -1;
    }
    if (!filepath) {
        KLOG_ERROR("Missing filepath");
        return -1;
    }
    if (argc && !argv) {
        KLOG_ERROR("Missing argv");
        return -1;
    }

    proc->filepath = copy_string(filepath);
    if (!proc->filepath) {
        KLOG_ERROR("Failed to copy filepath");
        return -1;
    }
    proc->argc = argc + 1;
    proc->argv = kmalloc(sizeof(char *) * (argc + 1));
    if (!proc->argv) {
        KLOG_ERROR("Failed to malloc process_t argv");
        kfree(proc->filepath);
        return -1;
    }

    proc->argv[0] = copy_string(filepath);
    if (!proc->argv[0]) {
        KLOG_ERROR("Failed to copy filepath to argv");
        kfree(proc->argv);
        kfree(proc->filepath);
        return -1;
    }

    for (int i = 0; i < argc; i++) {
        proc->argv[i + 1] = copy_string(argv[i]);
        if (!proc->argv[i + 1]) {
            KLOG_ERROR("Failed to copy arg %d", i);
            for (int j = 0; j < i + 1; j++) {
                kfree(proc->argv[i]);
            }
            kfree(proc->argv);
            kfree(proc->filepath);
            return -1;
        }
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
    // TODO use exec code or replace this with exec version of copy args
    const char * filename = "init";

    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        KLOG_ERROR("Failed to find file\n");
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
        KLOG_ERROR("Failed to create process for %s", filename);
        return 0;
    }

    if (process_load_heap(proc, buff, stat.size)) {
        KLOG_ERROR("Failed to load %s", filename);
        process_free(proc);
        return 0;
    }

    for (size_t i = 0; i < 1022; i++) {
        if (process_grow_stack(proc)) {
            KLOG_ERROR("Failed to grow process stack");
            return 0;
        }
    }

    copy_args(proc, filename, 0, 0);

    process_set_entrypoint(proc, proc_entry);
    process_add_pages(proc, 32);
    pm_add_proc(&get_kernel()->pm, proc);
    pm_set_foreground_proc(&get_kernel()->pm, proc->pid);

    tar_file_close(file);
    kfree(buff);

    return proc;
}
