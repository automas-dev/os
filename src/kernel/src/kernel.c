#include "kernel.h"

#include "commands.h"
#include "config.h"
#include "cpu/gdt.h"
#include "cpu/isr.h"
#include "cpu/mmu.h"
#include "cpu/ports.h"
#include "cpu/tss.h"
#include "defs.h"
#include "drivers/ata.h"
#include "drivers/keyboard.h"
#include "drivers/ramdisk.h"
#include "drivers/rtc.h"
#include "drivers/timer.h"
#include "drivers/vga.h"
#include "exec.h"
#include "io/file.h"
#include "kernel/boot_params.h"
#include "kernel/system_call_io.h"
#include "kernel/system_call_io_dir.h"
#include "kernel/system_call_io_file.h"
#include "kernel/system_call_mem.h"
#include "kernel/system_call_proc.h"
#include "kernel/system_call_stdio.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libk/defs.h"
#include "libk/sys_call.h"
#include "process.h"
#include "process_manager.h"
#include "ram.h"
#include "system_call.h"

static kernel_t __kernel;

extern _Noreturn void halt(void);

static void id_map_range(mmu_table_t * table, size_t start, size_t end);
static void id_map_page(mmu_table_t * table, size_t page);
static void cursor();
static void irq_install();
static int  kill(size_t argc, char ** argv);
static void map_first_table(mmu_table_t * table);

extern void jump_kernel_mode(void * fn);

static void setup_physical_memory();
static void setup_virtual_memory();
static void setup_tss();
static void setup_system_calls();

static void idle_loop();
static int  start_shell();

void kernel_main() {
    // 1. load vga driver
    init_vga(UINT2PTR(PADDR_VGA));
    // 1.1 clear screen
    vga_clear();

    // 2. setup memory
    kmemset(&__kernel, 0, sizeof(kernel_t));

    // 2.1 physical memory allocator
    setup_physical_memory();

    // 2.2 virtual memory (paging)
    setup_virtual_memory();

    // 3. enable paging
    mmu_enable_paging(__kernel.cr3);

    // 4. setup gdt (kernel + usr + tss)
    init_gdt();

    // 5. setup tss (empty)
    setup_tss();

    // 6. setup isr and idt
    isr_install();

    // 7. setup system calls
    setup_system_calls();

    // TODO why should the kernel need system calls?
    // Init kernel memory after system calls are registered
    memory_init(&__kernel.proc.memory, kernel_alloc_page);

    // 8. setup event bus
    // Create ebus for kernel (target of queue_event)
    if (ebus_create(&__kernel.event_queue, 4096)) {
        KPANIC("Failed to init ebus\n");
    }

    // 9. setup process manager
    pm_create(&__kernel.pm);

    scheduler_init(&__kernel.scheduler, &__kernel.pm);

    // 9.a create kernel process
    // Setup kernel process and add it to pm
    __kernel.pm.idle_task   = &__kernel.proc;
    __kernel.proc.next_proc = __kernel.pm.idle_task;
    pm_add_proc(&__kernel.pm, &__kernel.proc);

    // Create ebus for kernel proc
    if (ebus_create(&__kernel.proc.event_queue, 4096)) {
        KPANIC("Failed to init ebus\n");
    }

    // 10. setup irq
    // Init drivers and hardware interrupts
    irq_install();

    // 11. print welcome message
    vga_puts("Welcome to kernel v" PROJECT_VERSION "\n");

    // TODO is this needed here? Create it when it's needed
    ramdisk_create(4096);

    __kernel.disk = disk_open(0, DISK_DRIVER_ATA);
    if (!__kernel.disk) {
        KPANIC("Failed to open ATA disk");
    }

    __kernel.tar = tar_open(__kernel.disk);
    if (!__kernel.tar) {
        KPANIC("Failed to open tar");
    }

    start_shell();

    pm_resume_process(&__kernel.pm, __kernel.pm.idle_task->pid, 0);

    idle_loop();

    KPANIC("You shouldn't be here!");
}

static void setup_physical_memory() {
    __kernel.ram_table_addr = PADDR_RAM_TABLE;
    ram_init((void *)__kernel.ram_table_addr, (void *)VADDR_RAM_BITMASKS);

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
}

static void setup_virtual_memory() {
    __kernel.cr3     = PADDR_KERNEL_DIR;
    mmu_dir_t * pdir = (mmu_dir_t *)__kernel.cr3;
    mmu_dir_clear(pdir);

    // Init first table
    uint32_t first_table_addr = ram_page_palloc();
    mmu_dir_set(pdir, 0, first_table_addr, MMU_DIR_RW);

    // Map first table
    mmu_table_t * first_table = UINT2PTR(first_table_addr);
    mmu_table_clear(first_table);
    map_first_table(first_table);

    // Map last table to dir for access to tables
    mmu_dir_set(pdir, MMU_DIR_SIZE - 1, __kernel.cr3, MMU_DIR_RW);
}

static void setup_tss() {
    init_tss();

    // Kernel process used for memory allocation
    __kernel.proc.next_heap_page = ADDR2PAGE(VADDR_RAM_BITMASKS) + ram_region_table_count();
    __kernel.proc.cr3            = PADDR_KERNEL_DIR;
    __kernel.proc.esp0           = VADDR_ISR_STACK;
    __kernel.proc.state          = PROCESS_STATE_LOADED;
    set_active_task(&__kernel.proc);

    // Set initial ESP0 before first task switch
    tss_set_esp0(VADDR_ISR_STACK);
}

static void setup_system_calls() {
    init_system_call(IRQ16);
    system_call_register(SYS_INT_FAMILY_IO, sys_call_io_cb);
    system_call_register(SYS_INT_FAMILY_MEM, sys_call_mem_cb);
    system_call_register(SYS_INT_FAMILY_PROC, sys_call_proc_cb);
    system_call_register(SYS_INT_FAMILY_STDIO, sys_call_tmp_stdio_cb);
    system_call_register(SYS_INT_FAMILY_STDIO, sys_call_tmp_stdio_cb);
    system_call_register(SYS_INT_FAMILY_IO_FILE, sys_call_io_file_cb);
    system_call_register(SYS_INT_FAMILY_IO_DIR, sys_call_io_dir_cb);
}

static void idle_loop() {
    for (;;) {
        asm("hlt");

        while (cb_len(&__kernel.event_queue.queue) > 0) {
            ebus_event_t event;
            if (cb_pop(&__kernel.event_queue.queue, &event)) {
                KPANIC("Failed to pop from event queue");
            }

            switch (event.event_id) {
                case EBUS_EVENT_EXEC: {
                    int pid = kernel_exec(event.exec.filename, event.exec.argc, event.exec.argv);
                    if (pid > 0) {
                        ebus_event_t proc_event  = {0};
                        proc_event.event_id      = EBUS_EVENT_PROC_MADE;
                        proc_event.proc_made.pid = pid;
                        if (ebus_push(&__kernel.event_queue, &proc_event)) {
                            KPANIC("Ebus push failed");
                        }
                    }
                } break;

                case EBUS_EVENT_PROC_CLOSE: {
                    process_t * proc = kernel_find_pid(event.proc_close.pid);
                    if (!proc) {
                        KPANIC("Failed to find pid");
                    }
                    if (pm_remove_proc(&__kernel.pm, proc->pid)) {
                        KPANIC("Failed to remove process from pm");
                    }
                    process_free(proc);
                } break;

                default: {
                    if (pm_push_event(&__kernel.pm, &event)) {
                        KPANIC("Failed to push event to process manager");
                    }
                } break;
            }
        }

        yield();
    }
}

int kernel_exec(const char * filename, size_t argc, char ** argv) {
    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        puts("Failed to find file\n");
        return -1;
    }

    uint8_t * buff = kmalloc(stat.size);
    if (!buff) {
        return -1;
    }

    tar_fs_file_t * file = tar_file_open(kernel_get_tar(), filename);
    if (!file) {
        kfree(buff);
        return -1;
    }

    if (!tar_file_read(file, buff, stat.size)) {
        tar_file_close(file);
        kfree(buff);
        return -1;
    }

    int pid = command_exec(buff, stat.size, argc, argv);

    if (pid < 0) {
        tar_file_close(file);
        kfree(buff);
        return -1;
    }

    tar_file_close(file);
    kfree(buff);

    return pid;
}

static int start_shell() {
    char * filename = "shell";
    return kernel_exec(filename, 1, &filename);
}

void * kernel_alloc_page(size_t count) {
    return process_add_pages(get_active_task(), count);
}

int kernel_switch_task() {
    return scheduler_run(&__kernel.scheduler);
}

mmu_dir_t * get_kernel_dir() {
    return (mmu_dir_t *)__kernel.cr3;
}

mmu_table_t * get_kernel_table() {
    return (mmu_table_t *)VADDR_KERNEL_TABLE;
}

process_t * get_current_process() {
    return get_active_task();
}

ebus_t * get_kernel_ebus() {
    return &__kernel.event_queue;
}

disk_t * kernel_get_disk() {
    return __kernel.disk;
}

tar_fs_t * kernel_get_tar() {
    return __kernel.tar;
}

void tmp_register_signals_cb(signals_master_cb_t cb) {
    get_active_task()->signals_callback = cb;
    printf("Attached master signal callback at %p\n", get_active_task()->signals_callback);
}

int kernel_add_task(process_t * proc) {
    return pm_add_proc(&__kernel.pm, proc);
}

int kernel_next_task() {
    return pm_resume_process(&__kernel.pm, get_active_task()->pid, 0);
}

int kernel_close_process(process_t * proc) {
    if (!proc) {
        return -1;
    }

    proc->state = PROCESS_STATE_DEAD;

    process_t * next = pm_get_next(&__kernel.pm);
    if (!next) {
        next = __kernel.pm.idle_task;
    }

    return 0;
}

NO_RETURN void kernel_panic(const char * msg, const char * file, unsigned int line) {
    vga_color(VGA_FG_WHITE | VGA_BG_RED);
    vga_puts("[KERNEL PANIC]");
    if (file) {
        vga_putc('[');
        vga_puts(file);
        vga_puts("]:");
        vga_putu(line);
    }
    if (msg) {
        vga_putc(' ');
        vga_puts(msg);
    }
    vga_cursor_hide();
    halt();
}

kernel_t * get_kernel() {
    return &__kernel;
}

proc_man_t * kernel_get_proc_man() {
    return &__kernel.pm;
}

process_t * kernel_find_pid(int pid) {
    return pm_find_pid(&__kernel.pm, pid);
}

void * kmalloc(size_t size) {
    return memory_alloc(&__kernel.proc.memory, size);
}

void * krealloc(void * ptr, size_t size) {
    return memory_realloc(&__kernel.proc.memory, ptr, size);
}

void kfree(void * ptr) {
    memory_free(&__kernel.proc.memory, ptr);
}

static void cursor() {
    vga_cursor(3, 3);

    vga_cursor_hide();
    vga_cursor_show();
    vga_cursor(vga_cursor_row(), vga_cursor_col());
}

static void irq_install() {
    enable_interrupts();
    /* IRQ0: timer */
    init_timer(TIMER_FREQ_MS); // milliseconds
    /* IRQ1: keyboard */
    init_keyboard();
    /* IRQ14: ata disk */
    init_ata();
    /* IRQ8: real time clock */
    init_rtc(RTC_RATE_1024_HZ);
}

static int kill(size_t argc, char ** argv) {
    printf("Leaving process now\n");
    kernel_exit();
    KPANIC("Never return!");
    return 0;
}

static void map_first_table(mmu_table_t * table) {
    // null page 0
    mmu_table_set(table, 0, 0, 0);

    // Page Directory
    mmu_table_set(table, 1, __kernel.cr3, MMU_DIR_RW);

    // Create first table
    mmu_table_set(table, 2, __kernel.ram_table_addr, MMU_DIR_RW);

    // Stack
    id_map_range(table, 3, 6);

    // Kernel
    id_map_range(table, 7, 0x9e);

    // VGA
    id_map_page(table, 0xb8);

    // Kernel Table
    mmu_table_set(table, ADDR2PAGE(VADDR_KERNEL_TABLE), (uint32_t)table, MMU_TABLE_RW);

    // RAM region bitmasks
    ram_table_t * ram_table = (ram_table_t *)(__kernel.ram_table_addr);

    for (size_t i = 0; i < ram_region_table_count(); i++) {
        uint32_t bitmask_addr = ram_table->entries[i].addr_flags & MASK_ADDR;
        mmu_table_set(table, ADDR2PAGE(VADDR_RAM_BITMASKS) + i, bitmask_addr, MMU_TABLE_RW);
    }
}

static void id_map_range(mmu_table_t * table, size_t start, size_t end) {
    if (end > 1023) {
        KPANIC("End is past table limits");
        end = 1023;
    }

    while (start <= end) {
        id_map_page(table, start);
        start++;
    }
}

static void id_map_page(mmu_table_t * table, size_t page) {
    mmu_table_set(table, page, page << 12, MMU_TABLE_RW);
}
