#include "system_call.h"

#include "cpu/isr.h"
#include "drivers/vga.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libk/defs.h"
#include "process.h"

#undef SERVICE
#define SERVICE "SYSCALL"

#define MAX_CALLBACKS 0x100
static sys_call_handler_t __callbacks[MAX_CALLBACKS];

static void callback(registers_t * regs);

void system_call_init(uint8_t isr_interrupt_no) {
    if (!kmemset(__callbacks, 0, sizeof(__callbacks))) {
        KLOG_ERROR("Failed to clear memory of callback handlers array");
        KPANIC("Failed to clear callback handlers array");
    }
    KLOG_DEBUG("Registering interrupt handler on IRQ %u", isr_interrupt_no);
    register_interrupt_handler(isr_interrupt_no, callback);

    KLOG_DEBUG("Initialized system calls");
}

void system_call_register(uint16_t family, sys_call_handler_t handler) {
    KLOG_DEBUG("Registering handler for family 0x%02X", family);
    if (family >= MAX_CALLBACKS) {
        KLOG_ERROR("Cannot register handler for family 0x%02X, must be < 0x%X", family, MAX_CALLBACKS);
        PANIC("Out of range interrupt family");
    }
    __callbacks[family] = handler;
}

static void callback(registers_t * regs) {
    uint32_t res = 0;

    uint32_t call_id = regs->eax;
    uint16_t family  = (regs->eax >> 16);

    KLOG_TRACE("Received system call 0x%X", call_id);

    // if (family != 0x01 && family != 0x10) {
    //     process_t * proc = get_current_process();
    //     KLOG_DEBUG("Got system call 0x%04x from PID %u", (int)call_id, proc->pid);
    // }

    void * args_data = UINT2PTR(regs->ebx);

    sys_call_handler_t handler = __callbacks[family];

    if (handler) {
        res = handler(call_id, args_data, regs);
    }
    else {
        KLOG_ERROR("Failed to find handler for system call 0x%04X", call_id);
        PANIC("UNKNOWN INTERRUPT");
    }

    // Get access to stack push of eax
    uint32_t * ret = UINT2PTR(regs->esp - 4);
    *ret           = res;
}
