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
#include "kernel/time.h"
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
            kernel_switch_task();

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

        case SYS_CALL_EVENT_TIME: {
            enable_interrupts();
            kernel_switch_task();

            return time_s();
        } break;

        case SYS_CALL_EVENT_SLEEP: {
            struct _args {
                size_t ms;
                size_t us;
            } * args = (struct _args *)args_data;

            int timer_id = 0;
            if (args->ms) {
                KLOG_TRACE("Using millisecond sleep %u", args->ms);
                timer_id = time_start_timer_ms(args->ms);
            }
            else {
                KLOG_TRACE("Using microsecond sleep %u", args->us);
                timer_id = time_start_timer_us(args->us);
            }

            if (timer_id < 1) {
                KLOG_WARNING("Failed to start timer of %u ms\n", args->ms);
                return 1;
            }

            proc->next_event.event_id   = 0;
            proc->next_event.timer.id   = 0;
            proc->next_event.timer.time = 0;
            proc->filter_event.event_id = EBUS_EVENT_TIMER;
            proc->filter_event.timer.id = timer_id;
            proc->state                 = PROCESS_STATE_WAITING;

            do {
                enable_interrupts();
                kernel_switch_task();
                KLOG_TRACE("Back from timer %u", timer_id);
            } while (proc->next_event.timer.id != timer_id);
        } break;
    }

    return 0;
}
