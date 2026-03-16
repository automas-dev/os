#define KLOG_SERVICE "SYSCALL/EVENT"
// #define KLOG_LEVEL   KERNEL_LOG_LEVEL_TRACE

#include "kernel/system_call_event.h"

#include <stddef.h>

#include "defs.h"
#include "drivers/vga.h"
#include "ebus.h"
#include "exec.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libk/defs.h"
#include "process.h"

int sys_call_event_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }

        // Generic event pull, make another case block for more specific logic
        case SYS_CALL_EVENT_PULL: {
            struct _args {
                int            filter;
                ebus_event_t * event_out;
            } * args = (struct _args *)args_data;

            if (!args->filter) {
                KLOG_WARNING("Event pul system call with no filter");
                return 1;
            }

            proc->next_event.event_id   = 0;
            proc->filter_event.event_id = args->filter;
            proc->state                 = PROCESS_STATE_WAITING;

            enable_interrupts();
            process_t * next = pm_get_next(kernel_get_proc_man());
            if (pm_resume_process(kernel_get_proc_man(), next->pid)) {
                KPANIC("Failed to resume process");
            }

            proc = get_current_process();

            // args->filter doesn't appear to be valid here, why not?
            if (!(proc->next_event.event_id == proc->filter_event.event_id)) {
                KPANIC("Tried to resume process but the event does not match filter");
            }
            if (args->event_out) {
                kmemcpy(args->event_out, &proc->next_event, sizeof(ebus_event_t));
            }

            proc->filter_event.event_id = 0;
            proc->next_event.event_id   = 0;
        } break;
    }

    return 0;
}
