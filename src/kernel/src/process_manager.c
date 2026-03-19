#define KLOG_SERVICE "KERNEL/PROCESS_MANAGER"
// #define KLOG_LEVEL   KLOG_LEVEL_TRACE

#include "process_manager.h"

#include "drivers/keyboard.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"

int pm_create(proc_man_t * pm) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }

    if (!kmemset(pm, 0, sizeof(proc_man_t))) {
        KLOG_ERROR("Failed to clear process manager struct");
        return -1;
    }

    return 0;
}

process_t * pm_find_pid(proc_man_t * pm, int pid) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return 0;
    }
    if (pid < 0) {
        KLOG_WARNING("Find takes a pid >= 0, got %d", pid);
        return 0;
    }
    if (!pm->first_task) {
        KLOG_WARNING("No processes exist in process manager, can't find pid %d", pid);
        return 0;
    }

    if (pm->foreground_task && pm->foreground_task->pid == pid) {
        return pm->foreground_task;
    }

    process_t * proc = pm->first_task;
    if (proc->pid == pid) {
        return proc;
    }

    do {
        if (proc->pid == pid) {
            return proc;
        }

        proc = proc->next;
    } while (proc != pm->first_task);

    return 0;
}

int pm_add_proc(proc_man_t * pm, process_t * proc) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }
    if (!proc) {
        KLOG_ERROR("Process struct is a null pointer");
        return -1;
    }

    if (!pm->first_task) {
        KLOG_INFO("Assigning first process to be %u", proc->pid);
        pm->first_task      = proc;
        pm->foreground_task = proc;

        // Link to self
        proc->next = proc;
        proc->prev = proc;
        return 0;
    }

    if (pm_find_pid(pm, proc->pid)) {
        KLOG_WARNING("Tried to add process %u to manager, already added", proc->pid);
        return 0;
    }

    process_t * prev = pm->first_task->prev;
    KLOG_TRACE("Using process %u as previous to first task %u", prev->pid, pm->first_task->pid);

    prev->next           = proc;
    pm->first_task->prev = proc;

    proc->next = pm->first_task;
    proc->prev = prev;

    process_t * p = pm->first_task;
    do {
        KLOG_TRACE("Link %u n %u p %u", p->pid, p->next->pid, p->prev->pid);
        p = p->next;
    } while (p != pm->first_task);
    return 0;
}

int pm_remove_proc(proc_man_t * pm, int pid) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }
    if (pid < 0) {
        KLOG_WARNING("Remove process takes a pid >= 0, got %d", pid);
        return -1;
    }

    if (pid == get_active_task()->pid) {
        KLOG_ERROR("Trying to remove active task pid %d", pid);
        return -1;
    }

    process_t * proc = pm_find_pid(pm, pid);
    if (!proc) {
        KLOG_ERROR("Failed to find process for pid %u", pid);
        return -1;
    }

    if (proc == pm->first_task) {
        KLOG_ERROR("Cannot remove first process");
        return -1;
    }

    if (proc == pm->foreground_task) {
        KLOG_ERROR("Cannot remove foreground process");
        return -1;
    }

    return process_unlink(proc);
}

int pm_set_foreground_proc(proc_man_t * pm, int pid) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }
    if (pid < 0) {
        KLOG_WARNING("Set foreground process takes a pid >= 0, got %d", pid);
        return -1;
    }

    process_t * proc = pm_find_pid(pm, pid);
    if (!proc) {
        KLOG_ERROR("Failed to find process for pid %d", pid);
        return -1;
    }

    pm->foreground_task = proc;

    return 0;
}

int pm_resume_process(proc_man_t * pm, int pid) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }
    if (pid < 0) {
        KLOG_WARNING("Set foreground process takes a pid >= 0, got %d", pid);
        return -1;
    }
    // TODO event is not used

    process_t * proc = pm_find_pid(pm, pid);
    if (!proc) {
        KLOG_ERROR("Failed to find process for pid %d", pid);
        return -1;
    }

    pm->foreground_task = proc;

    if (proc->filter_event.event_id) {
        // TODO assert next_event has an event of the correct type
        // TODO push to process next_event instead of event_queue
        // TODO clear next_event before resuming process
        // TODO does process state need updating? - probably not
    }

    return process_resume(proc, 0);
}

process_t * pm_get_next(proc_man_t * pm) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return 0;
    }

    process_t * proc = pm->foreground_task->next;

    // KLOG_TRACE("Start looking for next process");

    do {
        // KLOG_TRACE("Looking at pid %u in state %u to see if it's ready", proc->pid, proc->state);

        if (PROCESS_STATE_LOADED <= proc->state <= PROCESS_STATE_DEAD) {
            if (!proc->filter_event.event_id) {
                // KLOG_TRACE("Process %u has no filter event, so it's ready", proc->pid);
                return proc;
            }
            // This handles the above case but is split for trace log
            if (proc->filter_event.event_id == proc->next_event.event_id) {
                KLOG_TRACE("Process %u has ready event %u", proc->pid, proc->next_event.event_id);
                return proc;
            }
            // KLOG_TRACE("Process %u is not ready, waiting for %u", proc->pid, proc->filter_event.event_id);
        }
        else {
            // KLOG_TRACE("Process with pid %u is not alive", proc->pid);
        }

        // KLOG_TRACE("Going to next process %u, fg is %u", proc->next->pid, pm->foreground_task->pid);
        proc = proc->next;
    } while (proc != pm->foreground_task->next);

    // KLOG_TRACE("Finish looking for next process");

    if (PROCESS_STATE_LOADED <= proc->state <= PROCESS_STATE_DEAD) {
        // KLOG_TRACE("Next process is the foreground process with pid %u", proc->pid);
        return proc;
    }

    KPANIC("Could not find process to resume");
}

int pm_push_event(proc_man_t * pm, ebus_event_t * event) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }
    if (!event) {
        KLOG_ERROR("Event is a null pointer");
        return -1;
    }
    if (!event->event_id) {
        KLOG_ERROR("Event id must be non-zero");
        return -1;
    }

    KLOG_TRACE("Start push event %u", event->event_id);

    process_t * proc = pm->first_task;

    do {
        if (proc->filter_event.event_id == event->event_id) {
            KLOG_TRACE("Push event %u to %u", event->event_id, proc->pid);
            if (proc->next_event.event_id) {
                KLOG_WARNING("Replacing event %u with %u for process %u", proc->next_event.event_id, event->event_id, proc->pid);
            }
            kmemcpy(&proc->next_event, event, sizeof(ebus_event_t));

            if (event->event_id == EBUS_EVENT_KEY) {
                KLOG_TRACE("Push scancode 0x%x results in 0x%x", event->key.scancode, proc->next_event.key.scancode);
            }

            if (proc->state == PROCESS_STATE_WAITING) {
                KLOG_TRACE("Setting process state to suspended for pid %u", proc->pid);
                proc->state = PROCESS_STATE_SUSPENDED;
            }
        }
        else {
            KLOG_TRACE("Event %u did not match pid %u waiting on %u", event->event_id, proc->pid, proc->filter_event.event_id);
        }

        proc = proc->next;
    } while (proc != pm->first_task);

    KLOG_TRACE("Finish push event %u", event->event_id);

    return 0;
}
