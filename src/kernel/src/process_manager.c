#include "process_manager.h"

#include "drivers/keyboard.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"

#undef SERVICE
#define SERVICE "KERNEL/PROCESS_MANAGER"

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
        KLOG_TRACE("Returning foreground task for pid %u", pid);
        return pm->foreground_task;
    }

    process_t * proc = pm->first_task;
    if (proc->pid == pid) {
        KLOG_TRACE("Returning first task for pid %u", pid);
        return proc;
    }
    proc = proc->next;

    while (proc != pm->first_task) {
        if (proc->pid == pid) {
            return proc;
        }

        proc = proc->next;
    }

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
    else if (pm->first_task->next == pm->first_task) {
        KLOG_DEBUG("Assigning second process to be %u", proc->pid);
        pm->first_task->next = proc;
        pm->first_task->prev = proc;

        proc->next = pm->first_task;
        proc->prev = pm->first_task;
    }

    KLOG_DEBUG("Linking last task %u to new process %u", proc->prev->pid, proc->pid);
    return process_link(proc->prev, proc);
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

int pm_resume_process(proc_man_t * pm, int pid, ebus_event_t * event) {
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

    return process_resume(proc, event);
}

process_t * pm_get_next(proc_man_t * pm) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return 0;
    }

    process_t * proc = pm->foreground_task->next;

    while (proc != pm->foreground_task) {
        if (PROCESS_STATE_LOADED <= proc->state <= PROCESS_STATE_DEAD) {
            uint32_t filter_event = 0;
            if (ebus_queue_size(&proc->event_queue) > 0) {
                ebus_event_t event;
                if (ebus_peek(&proc->event_queue, &event)) {
                    filter_event = event.event_id;
                    KLOG_TRACE("Process %u has ready event of type %u", proc->pid, event.event_id);
                }
                else {
                    KLOG_ERROR("Failed to peek at process event queue which has length %u", ebus_queue_size(&proc->event_queue));
                }
            }

            if (!proc->filter_event || proc->filter_event == filter_event) {
                // TODO need to pop event from queue, then remove this if block
                if (filter_event) {
                    KLOG_WARNING("YOU NEED TO POP EBUS EVENT %u", filter_event);
                }
                return proc;
            }
            else {
                KLOG_TRACE("Process with pid %u does not match filter event %u, waiting for %u", proc->pid, filter_event, proc->filter_event);
            }
        }
        else {
            KLOG_TRACE("Process with pid %u is not alive", proc->pid);
        }

        proc = proc->next;
    };

    if (PROCESS_STATE_LOADED <= proc->state <= PROCESS_STATE_DEAD) {
        KLOG_TRACE("Next process is the foreground process with pid %u", proc->pid);
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

    if (pm->first_task->filter_event == event->event_id) {
        if (ebus_push(&pm->first_task->event_queue, event)) {
            KLOG_ERROR("Failed to push event of type %u to first task %u", event->event_id, pm->first_task->pid);
            return -1;
        }
    }

    process_t * proc = pm->first_task->next;

    while (proc != pm->first_task) {
        if (proc->filter_event == event->event_id) {
            if (ebus_push(&proc->event_queue, event)) {
                KLOG_ERROR("Failed to push event of type %u to process %u", event->event_id, proc->pid);
                return -1;
            }

            if (proc->state == PROCESS_STATE_WAITING) {
                KLOG_TRACE("Setting process state to suspended for pid %u", proc->pid);
                proc->state = PROCESS_STATE_SUSPENDED;
            }
        }

        proc = proc->next;
    }

    return 0;
}
