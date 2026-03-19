/**
 * @brief Higher kernel operates in virtual memory after paging is enabled.
 *
 * Documentation moved to design/boot_stages.md
 */
#define KLOG_SERVICE "KERNEL"

#include "kernel.h"

#include "defs.h"
#include "drivers/ata.h"
#include "drivers/keyboard.h"
#include "drivers/ram.h"
#include "drivers/ramdisk.h"
#include "drivers/rtc.h"
#include "exec.h"
#include "kernel/logs.h"
#include "kernel/panic.h"
#include "kernel/system_call_event.h"
#include "kernel/system_call_io.h"
#include "kernel/system_call_mem.h"
#include "kernel/system_call_proc.h"
#include "kernel/time.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libk/defs.h"

static kernel_t __kernel;

extern _Noreturn void halt(void);

static void irq_install();
static void setup_system_calls();

void kernel_init() {
    KLOG_INFO("Kernel Start");

    // 8.1 Clear kernel struct
    kmemset(&__kernel, 0, sizeof(kernel_t));

    __kernel.esp0 = VADDR_ISR_STACK;

    // 8.2 Install ISR and IDT
    isr_install();
    KLOG_DEBUG("isr init finished");

    // 8.3 Setup system calls
    setup_system_calls();
    KLOG_DEBUG("system call init finished");

    // 8.4 Initialize kmalloc
    kmalloc_init(ADDR2PAGE(VADDR_RAM_BITMASKS) + ram_region_table_count());
    KLOG_DEBUG("kmalloc init finished");

    // TODO why should the kernel need system calls?
    // Init kernel memory after system calls are registered
    // memory_init(&__kernel.proc.memory, kernel_alloc_page);
    // KLOG_DEBUG("memory init finished");

    // 8.5 Setup event bus
    // Create ebus for kernel (target of queue_event)
    if (ebus_create(&__kernel.event_queue, 4096)) {
        KPANIC("Failed to init ebus\n");
    }

    _libc_config_queue_event_call(kernel_queue_event);
    KLOG_DEBUG("ebus init finished");

    // 8.6 Create process manager
    pm_create(&__kernel.pm);
    KLOG_DEBUG("process manager init finished");

    // 8.7 Initialize Scheduler
    scheduler_init(&__kernel.scheduler, &__kernel.pm);
    KLOG_DEBUG("scheduler init finished");

    // 8.8 Install IRQ
    // Init drivers and hardware interrupts
    // TODO move earlier (maybe after isr install) to get time for logs
    irq_install();
    KLOG_DEBUG("irq init finished");

    // 8.9 Enable time in kernel logs
    kernel_log_time_enable();
    KLOG_DEBUG("enabled kernel log time");

    // 8.10 Mount disk
    __kernel.disk = disk_open(0, DISK_DRIVER_ATA);
    if (!__kernel.disk) {
        KPANIC("Failed to open ATA disk");
    }
    KLOG_DEBUG("open ata disk finished");

    // 8.11 Mount filesystem
    __kernel.tar = tar_open(__kernel.disk);
    if (!__kernel.tar) {
        KPANIC("Failed to open tar");
    }
    KLOG_DEBUG("open tar fs finished");
}

static void setup_system_calls() {
    system_call_init(IRQ16);
    system_call_register(SYS_CALL_FAMILY_IO, sys_call_io_cb);
    system_call_register(SYS_CALL_FAMILY_MEM, sys_call_mem_cb);
    system_call_register(SYS_CALL_FAMILY_PROC, sys_call_proc_cb);
    system_call_register(SYS_CALL_FAMILY_EVENT, sys_call_event_cb);
}

int kernel_exec(const char * filename, size_t argc, char ** argv) {
    if (!filename) {
        KLOG_ERROR("Missing filename");
        return -1;
    }
    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        KLOG_ERROR("Failed to find file %s to execute", filename);
        return -1;
    }

    uint8_t * buff = kmalloc(stat.size);
    if (!buff) {
        KLOG_ERROR("Failed to malloc buffer of size %u", stat.size);
        return -1;
    }

    tar_fs_file_t * file = tar_file_open(kernel_get_tar(), filename);
    if (!file) {
        KLOG_ERROR("Failed to open file %s", filename);
        kfree(buff);
        return -1;
    }

    if (!tar_file_read(file, buff, stat.size)) {
        KLOG_ERROR("Failed to read from file %s", filename);
        tar_file_close(file);
        kfree(buff);
        return -1;
    }

    int pid = command_exec(buff, filename, stat.size, argc, argv);

    if (pid < 0) {
        KLOG_ERROR("Failed to execute file %s", filename);
        tar_file_close(file);
        kfree(buff);
        return -1;
    }

    tar_file_close(file);
    kfree(buff);

    return pid;
}

void kernel_switch_task() {
    if (scheduler_run(&__kernel.scheduler)) {
        KPANIC("Failed to switch tasks");
    }
}

process_t * get_current_process() {
    return get_active_task();
}

ebus_t * get_kernel_ebus() {
    return &__kernel.event_queue;
}

void kernel_queue_event(ebus_event_t * event) {
    if (ebus_push(&__kernel.event_queue, event)) {
        KLOG_ERROR("Failed to push event to kernel event queue");
        return;
    }
    if (pm_push_event(&__kernel.pm, event)) {
        KLOG_ERROR("Failed to push event to process manager");
    }
}

disk_t * kernel_get_disk() {
    return __kernel.disk;
}

tar_fs_t * kernel_get_tar() {
    return __kernel.tar;
}

void tmp_register_signals_cb(signals_master_cb_t cb) {
    get_active_task()->signals_callback = cb;
    KLOG_DEBUG("Attached master signal callback at %p\n", get_active_task()->signals_callback);
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

static void irq_install() {
    enable_interrupts();
    KLOG_TRACE("interrupts enabled");
    /* IRQ0: timer */
    time_init(TIMER_FREQ_MS); // milliseconds
    KLOG_TRACE("pit init finished");
    /* IRQ1: keyboard */
    keyboard_init();
    KLOG_TRACE("keyboard init finished");
    /* IRQ14: ata disk */
    ata_init();
    KLOG_TRACE("ata init finished");
    /* IRQ8: real time clock */
    rtc_init(RTC_RATE_1024_HZ);
    KLOG_TRACE("rtc init finished");
}
