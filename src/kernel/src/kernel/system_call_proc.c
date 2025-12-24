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

int sys_call_proc_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    int res = 0;

    switch (int_no) {

            // TODO this isn't fully updated with task switching

        case SYS_INT_PROC_EXIT: {
            KLOGS_DEBUG("SC_PROC", "System call proc exit");
            struct _args {
                uint8_t code;
            } * args = (struct _args *)args_data;
            printf("Proc exit with code %u\n", args->code);
            process_t * proc = get_current_process();
            enable_interrupts();

            ebus_event_t event           = {0};
            event.event_id               = EBUS_EVENT_PROC_CLOSE;
            event.proc_close.pid         = get_active_task()->pid;
            event.proc_close.status_code = args->code;

            queue_event(&event);
            kernel_switch_task();
            KPANIC("Unexpected return from kernel_switch_task");
        } break;

            // TODO this isn't fully updated with task switching

        case SYS_INT_PROC_ABORT: {
            KLOGS_DEBUG("SC_PROC", "System call proc abort");
            struct _args {
                uint8_t      code;
                const char * msg;
            } * args = (struct _args *)args_data;
            printf("Proc abort with code %u\n", args->code);
            puts(args->msg);
            process_t * proc = get_current_process();
            enable_interrupts();

            ebus_event_t event           = {0};
            event.event_id               = EBUS_EVENT_PROC_CLOSE;
            event.proc_close.pid         = get_active_task()->pid;
            event.proc_close.status_code = args->code;

            queue_event(&event);
            kernel_switch_task();
            KPANIC("Unexpected return from kernel_switch_task");
        } break;

        case SYS_INT_PROC_PANIC: {
            KLOGS_DEBUG("SC_PROC", "System call proc panic");
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

        case SYS_INT_PROC_REG_SIG: {
            KLOGS_DEBUG("SC_PROC", "System call proc sig");
            struct _args {
                signals_master_cb_t cb;
            } * args = (struct _args *)args_data;
            tmp_register_signals_cb(args->cb);
        } break;

        case SYS_INT_PROC_GETPID: {
            // KLOGS_DEBUG("SC_PROC", "System call proc getpid");
            process_t * p = get_current_process();
            if (!p) {
                KPANIC("Failed to find current process");
            }
            res = p->pid;
        } break;

        case SYS_INT_PROC_QUEUE_EVENT: {
            // KLOGS_DEBUG("SC_PROC", "System call proc queue event");
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

        case SYS_INT_PROC_YIELD: {
            // KLOGS_DEBUG("SC_PROC", "System call proc yield");
            struct _args {
                int            filter;
                ebus_event_t * event_out;
            } * args = (struct _args *)args_data;

            // TODO clear iret from stack?
            process_t * proc   = get_current_process();
            proc->filter_event = args->filter;
            proc->state        = (args->filter ? PROCESS_STATE_WAITING : PROCESS_STATE_SUSPENDED);
            // process_yield(proc, regs->esp, regs->eip, args->filter);
            enable_interrupts();
            process_t * next      = pm_get_next(kernel_get_proc_man());
            int         has_event = 0;
            if (ebus_queue_size(&next->event_queue) > 0) {
                // KLOGS_DEBUG("SC_PROC", "Got %u events", ebus_queue_size(&next->event_queue));
                if (ebus_pop(&next->event_queue, args->event_out)) {
                    KPANIC("Yea, that didn't work");
                }
                has_event = 1;
            }
            // KLOGS_DEBUG("SC_PROC", "Switching from process %u to %u", proc->pid, next->pid);
            if (pm_resume_process(kernel_get_proc_man(), next->pid, 0)) {
                KPANIC("Failed to resume process");
            }

            // TODO return 1 for event out
            // proc = get_current_process();
            // if (ebus_queue_size(&proc->event_queue) > 0) {
            //     if (ebus_pop(&proc->event_queue, args->event_out)) {
            //         return -1;
            //     }
            //     if (args->event_out) {
            //         return args->event_out->event_id;
            //     }
            // }
            return has_event;
        } break;

        case SYS_INT_PROC_EXEC: {
            struct _args {
                const char * filename;
                size_t       argc;
                char **      argv;
            } * args = (struct _args *)args_data;
            KLOGS_DEBUG("SC_PROC", "System call proc exec \"%s\" argc=%d", args->filename, args->argc);

            return kernel_exec(args->filename, args->argc, args->argv);
        } break;

        case SYS_INT_PROC_SET_FOREGROUND: {
            struct _args {
                int pid;
            } * args = (struct _args *)args_data;
            KLOGS_DEBUG("SC_PROC", "System call set foreground pid %d", args->pid);

            return pm_set_foreground_proc(kernel_get_proc_man(), args->pid);
        } break;
    }

    return res;
}
