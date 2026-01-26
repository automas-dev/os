#include "process_manager.h"

#include "drivers/keyboard.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"

#undef SERVICE
#define SERVICE "KERNEL/PROCESS_MANAGER"

static int pid_arr_index(arr_t * arr, int pid);

int pm_create(proc_man_t * pm) {
    if (!pm) {
        KLOG_ERROR("Process manager struct is a null pointer");
        return -1;
    }

    if (!kmemset(pm, 0, sizeof(proc_man_t))) {
        KLOG_ERROR("Failed to clear process manager struct");
        return -1;
    }

    if (arr_create(&pm->task_list, 4, sizeof(process_t *))) {
        KLOG_ERROR("Failed to create process manager task list array");
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

    int i = pid_arr_index(&pm->task_list, pid);
    if (i < 0) {
        // TODO should this be warning or debug / trace?
        KLOG_WARNING("Failed to find process pid %d", pid);
        return 0;
    }

    process_t * proc;
    if (arr_get(&pm->task_list, i, &proc)) {
        KLOG_ERROR("Failed to get process for index %d", i);
        return 0;
    }

    return proc;
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

    if (arr_insert(&pm->task_list, arr_size(&pm->task_list), &proc)) {
        KLOG_ERROR("Failed to insert process pid %u into process manager", proc->pid);
        return -1;
    }

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

    int i = pid_arr_index(&pm->task_list, pid);
    if (i < 0 || arr_remove(&pm->task_list, i, 0)) {
        KLOG_ERROR("Failed to remove process from task list array");
        return -1;
    }

    return 0;
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

    int i = pid_arr_index(&pm->task_list, get_active_task()->pid);
    if (i < 0) {
        KLOG_ERROR("Failed to fin index of current task");
        return 0;
    }

    int start_i = i++;

    while (i != start_i) {
        if (i >= arr_size(&pm->task_list)) {
            i = 0;
        }

        process_t * proc;
        if (arr_get(&pm->task_list, i, &proc)) {
            KPANIC("Failed to get proc");
            return 0;
        }

        KLOG_TRACE("PID %u is state %x", proc->pid, proc->state);

        // if (proc->state == PROCESS_STATE_WAITING_STDIN) {
        //     KLOG_DEBUG("Process waiting for stdin");
        //     if (io_buffer_length(proc->io_buffer) > 0) {
        //         KLOG_DEBUG("Process is waiting for stdin and has %u ready", io_buffer_length(proc->io_buffer));
        //         return proc;
        //     }
        // }

        if (proc->state == PROCESS_STATE_WAITING && ebus_queue_size(&proc->event_queue) > 0) {
            ebus_event_t event;
            if (ebus_peek(&proc->event_queue, &event) > 0) {
                proc->state = PROCESS_STATE_SUSPENDED;
            }
        }

        if (proc->state == PROCESS_STATE_LOADED || proc->state == PROCESS_STATE_SUSPENDED || proc->state == PROCESS_STATE_RUNNING) {
            return proc;
        }

        i++;
    }

    process_t * active = get_active_task();
    if (PROCESS_STATE_LOADED <= active->state <= PROCESS_STATE_DEAD) {
        return active;
    }

    KPANIC("Process Loop!");

    return 0;
}

static int pid_arr_index(arr_t * arr, int pid) {
    if (!arr) {
        KLOG_ERROR("Array is a null pointer");
        return -1;
    }
    if (pid < 0) {
        KLOG_WARNING("PID array find index takes a pid >= 0, got %d", pid);
        return -1;
    }

    for (int i = 0; i < arr_size(arr); i++) {
        process_t * proc;
        arr_get(arr, i, &proc);

        if (proc->pid == pid) {
            return i;
        }
    }

    KLOG_ERROR("Failed to find index of pid %u", pid);

    return -1;
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

    if (event->event_id = EBUS_EVENT_KEY) {
        process_t * foreground = pm->foreground_task;

        KLOG_TRACE("Foreground is %u", foreground->pid);

        // if (event->key.event == KEY_EVENT_PRESS) {
        //     if (io_buffer_push(foreground->io_buffer, event->key.c)) {
        //         KLOG_WARNING("Failed to push key into io buffer");
        //         return -1;
        //     }
        //     KLOG_TRACE("IO buffer is size %u", io_buffer_length(foreground->io_buffer));
        // }

        if (ebus_push(&foreground->event_queue, event)) {
            KLOG_ERROR("Failed to push event to ebus for foreground process");
            return -1;
        }
    }
    else {
        process_t * active = get_active_task();

        for (size_t i = 0; i < arr_size(&pm->task_list); i++) {
            process_t * proc;
            arr_get(&pm->task_list, i, &proc);

            if (proc->pid == active->pid) {
                continue;
            }

            if (proc->state <= PROCESS_STATE_LOADED || proc->state >= PROCESS_STATE_DEAD) {
                continue;
            }

            if (proc->filter_event == event->event_id) {
                KLOG_DEBUG("Process %u was waiting for %u and got it", proc->pid, proc->filter_event);
                if (ebus_push(&proc->event_queue, event)) {
                    KLOG_ERROR("Failed to push event to ebus for process %u", proc->pid);
                    return -1;
                }

                if (proc->state == PROCESS_STATE_WAITING) {
                    proc->state = PROCESS_STATE_SUSPENDED;
                }
            }
        }
    }

    return 0;
}
