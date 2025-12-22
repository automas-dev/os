#include "kernel/system_call_proc.h"

#include <stddef.h>

#include "defs.h"
#include "ebus.h"
#include "exec.h"
#include "kernel.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libk/defs.h"
#include "process.h"
#include "drivers/vga.h"

int sys_call_proc_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    int res = 0;

    switch (int_no) {

            // TODO this isn't fully updated with task switching

        case SYS_INT_PROC_EXIT: {
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
            KPANIC("Unexpected return from kernel_close_process");
        } break;

            // TODO this isn't fully updated with task switching

        case SYS_INT_PROC_ABORT: {
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
            KPANIC("Unexpected return from kernel_close_process");
        } break;

        case SYS_INT_PROC_PANIC: {
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
            struct _args {
                signals_master_cb_t cb;
            } * args = (struct _args *)args_data;
            tmp_register_signals_cb(args->cb);
        } break;

        case SYS_INT_PROC_GETPID: {
            process_t * p = get_current_process();
            if (!p) {
                KPANIC("Failed to find current process");
            }
            res = p->pid;
        } break;

        case SYS_INT_PROC_QUEUE_EVENT: {
            struct _args {
                ebus_event_t * event;
            } * args = (struct _args *)args_data;

            if (!args->event) {
                return -1;
            }

            process_t * proc        = get_current_process();
            args->event->source_pid = proc->pid;

            ebus_push(get_kernel_ebus(), args->event);
        } break;

        case SYS_INT_PROC_YIELD: {
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
            process_t * next = pm_get_next(kernel_get_proc_man());
            if (pm_resume_process(kernel_get_proc_man(), next->pid, 0)) {
                KPANIC("Failed to resume process");
            }
            proc = get_current_process();
            if (ebus_queue_size(&proc->event_queue) > 0) {
                if (ebus_pop(&proc->event_queue, args->event_out)) {
                    return -1;
                }
                if (args->event_out) {
                    return args->event_out->event_id;
                }
            }
            return 0;
        } break;

        case SYS_INT_PROC_EXEC: {
            struct _args {
                const char * filename;
                size_t       argc;
                char **      argv;
            } * args = (struct _args *)args_data;

            if (args->argc < 1) {
                return -1;
            }

            char * copy_filename = kmalloc(kstrlen(args->filename) + 1);
            kmemcpy(copy_filename, args->filename, sizeof(args->filename) + 1);

            char ** copy = kmalloc(sizeof(char *) * args->argc);
            for (size_t i = 0; i < args->argc; i++) {
                size_t len = kstrlen(args->argv[i]) + 1;
                copy[i]    = kmalloc(len);
                kmemcpy(&copy[i], &args->argv[i], len);
            }

            ebus_event_t event  = {0};
            event.event_id      = EBUS_EVENT_EXEC;
            event.exec.filename = copy_filename;
            event.exec.argc     = args->argc;
            event.exec.argv     = copy;

            queue_event(&event);

            for (;;) {
                ebus_event_t event;

                int eid = pull_event(EBUS_EVENT_PROC_MADE, &event);

                if (eid == EBUS_EVENT_PROC_MADE) {
                    return event.proc_made.pid;
                }
            }
        } break;
    }

    return res;
}
