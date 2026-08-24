#define KLOG_SERVICE "KERNEL/SYSTEM_CALL_PROC"

#include "kernel/system_call_proc.h"

#include <stddef.h>

#include "defs.h"
#include "drivers/vga.h"
#include "ebus.h"
#include "exec.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "kernel/scheduler.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libk/defs.h"
#include "process.h"

int sys_call_proc_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }

        case SYS_CALL_PROC_EXIT: {
            struct _args {
                int code;
            } * args = (struct _args *)args_data;
            KLOG_DEBUG("Setting process %u state from %u to %u with status code %d", proc->pid, proc->state, PROCESS_STATE_DEAD, args->code);

            proc->state       = PROCESS_STATE_DEAD;
            proc->status_code = args->code;

            ebus_event_t event;
            event.event_id               = EBUS_EVENT_PROC_CLOSE;
            event.proc_close.pid         = proc->pid;
            event.proc_close.status_code = args->code;
            kernel_queue_event(&event);

            enable_interrupts();
            kernel_switch_task();

            KPANIC("Unexpected return from task switch in SYS_CALL_PROC_EXIT");
        } break;

        case SYS_CALL_PROC_ABORT: {
            KLOG_DEBUG("System call proc abort");
            struct _args {
                int          code;
                const char * msg;
            } * args = (struct _args *)args_data;
            printf("Proc abort with code %u\n", args->code);
            puts(args->msg);
            proc->state = PROCESS_STATE_DEAD;

            ebus_event_t event;
            event.event_id               = EBUS_EVENT_PROC_CLOSE;
            event.proc_close.pid         = proc->pid;
            event.proc_close.status_code = args->code;
            kernel_queue_event(&event);

            enable_interrupts();
            kernel_switch_task();

            KPANIC("Unexpected return from task switch in SYS_CALL_PROC_ABORT");
        } break;

        case SYS_CALL_PROC_PANIC: {
            struct _args {
                const char * msg;
                const char * file;
                unsigned int line;
            } * args = (struct _args *)args_data;

            // Default empty string if not provided by process
            const char * file = "";
            if (!file) {
                file = args->file;
            }

            // Default empty string if not provided by process
            const char * msg = "";
            if (!msg) {
                msg = args->msg;
            }

            KLOG_WARNING("Process %u panicked in %s at line %s: %s", proc->pid, file, args->line, msg);

            proc->state = PROCESS_STATE_DEAD;

            ebus_event_t event;
            event.event_id               = EBUS_EVENT_PROC_CLOSE;
            event.proc_close.pid         = proc->pid;
            event.proc_close.status_code = -1;
            kernel_queue_event(&event);

            enable_interrupts();
            kernel_switch_task();

            KPANIC("Unexpected return from task switch in SYS_CALL_PROC_PANIC");
        } break;

        case SYS_CALL_PROC_REG_SIG: {
            KLOG_DEBUG("System call proc sig");
            struct _args {
                signals_master_cb_t cb;
            } * args = (struct _args *)args_data;
            tmp_register_signals_cb(args->cb);
        } break;

        case SYS_CALL_PROC_GETPID: {
            // KLOG_DEBUG("System call proc getpid");
            process_t * p = get_current_process();
            if (!p) {
                KPANIC("Failed to find current process");
            }
            return p->pid;
        } break;

        case SYS_CALL_PROC_QUEUE_EVENT: {
            // KLOG_DEBUG("System call proc queue event");
            struct _args {
                ebus_event_t * event;
            } * args = (struct _args *)args_data;

            if (!args->event) {
                return -1;
            }

            process_t * proc        = get_current_process();
            args->event->source_pid = proc->pid;

            kernel_queue_event(args->event);
        } break;

        case SYS_CALL_PROC_YIELD: {
            proc->filter_event.event_id = 0;
            proc->next_event.event_id   = 0;
            proc->state                 = PROCESS_STATE_SUSPENDED;

            enable_interrupts();
            kernel_switch_task();

            proc->filter_event.event_id = 0;
            proc->next_event.event_id   = 0;
        } break;

        case SYS_CALL_PROC_EXEC: {
            struct _args {
                const char * filename;
                size_t       argc;
                char **      argv;
            } * args = (struct _args *)args_data;
            KLOG_DEBUG("System call proc exec \"%s\" argc=%d", args->filename, args->argc);

            return kernel_exec(args->filename, args->argc, args->argv);
        } break;

        case SYS_CALL_PROC_SET_FOREGROUND: {
            struct _args {
                int pid;
            } * args = (struct _args *)args_data;
            KLOG_DEBUG("System call set foreground pid %d", args->pid);

            return pm_set_foreground_proc(kernel_get_proc_man(), args->pid);
        } break;

        case SYS_CALL_PROC_WAIT_PID: {
            struct _args {
                int   pid;
                int * exit_status;
            } * args = (struct _args *)args_data;

            process_t * child = pm_find_pid(kernel_get_proc_man(), args->pid);
            if (!child) {
                KLOG_WARNING("Tried to wait on pid %u which is not found", args->pid);
                return -1;
            }

            if (child->state >= PROCESS_STATE_DEAD) {
                KLOG_WARNING("Tried to wait on pid %u which is dead in state %u", args->pid, child->state);
                return -1;
            }

            proc->next_event.event_id         = 0;
            proc->filter_event.event_id       = EBUS_EVENT_PROC_CLOSE;
            proc->filter_event.proc_close.pid = args->pid;
            proc->state                       = PROCESS_STATE_WAITING;

            for (;;) {
                enable_interrupts();
                kernel_switch_task();
                if (proc->next_event.proc_close.pid == args->pid) {
                    KLOG_DEBUG("Waited process %u has closed, waiting process is %u", args->pid, proc->pid);
                    break;
                }
                proc->next_event.event_id = 0;
            }

            if (!(proc->next_event.event_id == proc->filter_event.event_id)) {
                KPANIC("Tried to resume process but the event does not match filter");
            }
            if (args->exit_status) {
                KLOG_TRACE("Sending exist status %d of pid %u back to caller", proc->next_event.proc_close.pid, proc->next_event.proc_close.status_code);
                *args->exit_status = proc->next_event.proc_close.status_code;
            }

            proc->filter_event.event_id = 0;
            proc->next_event.event_id   = 0;
        } break;
    }

    return 0;
}
