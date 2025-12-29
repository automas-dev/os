#include "kernel/scheduler.h"

#include "ebus.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/string.h"

static void idle();

int scheduler_init(scheduler_t * scheduler, proc_man_t * pm) {
    if (!scheduler || !pm) {
        return -1;
    }

    kmemset(scheduler, 0, sizeof(scheduler_t));

    scheduler->pm = pm;

    return 0;
}

// TODO this is just a copy of the kernel / process manager current behavior
int scheduler_run(scheduler_t * scheduler) {
    if (!scheduler) {
        return -1;
    }

    if (cb_len(&get_kernel()->event_queue.queue) > 0) {
        KLOGS_DEBUG("Scheduler", "There are %u events ready", cb_len(&get_kernel()->event_queue.queue));
        ebus_event_t event;

        if (cb_pop(&get_kernel()->event_queue.queue, &event) < 0) {
            KPANIC("Failed to pop from event queue");
        }

        switch (event.event_id) {
            case EBUS_EVENT_EXEC: {
                int pid = kernel_exec(event.exec.filename, event.exec.argc, event.exec.argv);
                if (pid > 0) {
                    ebus_event_t proc_event  = {0};
                    proc_event.event_id      = EBUS_EVENT_PROC_MADE;
                    proc_event.proc_made.pid = pid;
                    if (ebus_push(&get_kernel()->event_queue, &proc_event)) {
                        KPANIC("Ebus push failed");
                    }
                }
            } break;

            case EBUS_EVENT_PROC_CLOSE: {
                process_t * proc = kernel_find_pid(event.proc_close.pid);
                if (!proc) {
                    KPANIC("Failed to find pid");
                }
                if (pm_remove_proc(&get_kernel()->pm, proc->pid)) {
                    KPANIC("Failed to remove process from pm");
                }
                process_free(proc);
            } break;

            default: {
                if (pm_push_event(&get_kernel()->pm, &event)) {
                    KPANIC("Failed to push event to process manager");
                }
            } break;
        }
    }

    process_t * next = pm_get_next(scheduler->pm);

    if (next) {
        // TODO ebus events
        process_resume(next, 0);
        KPANIC("PROCESS SHOULD NOT RETURN TO SCHEDULER!");
    }
    else {
        idle();
    }
}

static void idle() {
    asm("hlt");
}
