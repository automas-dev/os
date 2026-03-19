#define KLOG_SERVICE "SYSCALL/PROCESS"

#include "kernel/system_call_proc.h"

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

int sys_call_proc_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }

        case SYS_CALL_PROC_EXIT: {
            KLOG_DEBUG("Setting process %u state from %u to %u", proc->pid, proc->state, PROCESS_STATE_DEAD);
            proc->state = PROCESS_STATE_DEAD;

            enable_interrupts();
            process_t * next = pm_get_next(kernel_get_proc_man());
            KLOG_DEBUG("Next after %u is %u in state %u", proc->pid, next->pid, next->state);
            if (pm_resume_process(kernel_get_proc_man(), next->pid)) {
                KPANIC("Failed to resume process");
            }

            KPANIC("Unexpected return from pm_resume_process in SYS_CALL_PROC_EXIT");
        } break;

        case SYS_CALL_PROC_ABORT: {
            KLOG_DEBUG("System call proc abort");
            struct _args {
                uint8_t      code;
                const char * msg;
            } * args = (struct _args *)args_data;
            printf("Proc abort with code %u\n", args->code);
            puts(args->msg);
            proc->state = PROCESS_STATE_DEAD;

            enable_interrupts();
            process_t * next = pm_get_next(kernel_get_proc_man());
            KLOG_DEBUG("Next after %u is %u in state %u", proc->pid, next->pid, next->state);
            if (pm_resume_process(kernel_get_proc_man(), next->pid)) {
                KPANIC("Failed to resume process");
            }

            KPANIC("Unexpected return from pm_resume_process in SYS_CALL_PROC_ABORT");
        } break;

        case SYS_CALL_PROC_PANIC: {
            KLOG_DEBUG("System call proc panic");
            struct _args {
                const char * msg;
                const char * file;
                unsigned int line;
            } * args = (struct _args *)args_data;
            vga_color(VGA_FG_WHITE | VGA_BG_RED);
            vga_puts("[PANIC]");
            if (args->file) {
                vga_putc('[');
                vga_puts(args->file);
                vga_puts("]:");
                vga_putu(args->line);
            }
            if (args->msg) {
                vga_putc(' ');
                vga_puts(args->msg);
            }
            vga_cursor_hide();
            asm("cli");
            for (;;) {
                asm("hlt");
            }
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
            process_t * next = pm_get_next(kernel_get_proc_man());
            if (pm_resume_process(kernel_get_proc_man(), next->pid)) {
                KPANIC("Failed to resume process");
            }
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
    }

    return 0;
}
