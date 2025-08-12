#ifndef KERNEL_PROCESS_MANAGER_H
#define KERNEL_PROCESS_MANAGER_H

#include <stddef.h>
#include <stdint.h>

#include "ebus.h"
#include "libc/datastruct/array.h"
#include "process.h"

typedef struct _proc_man {
    arr_t       task_list; // process_t *
    process_t * idle_task;
} proc_man_t;

int pm_create(proc_man_t * pm);

// TODO pm_free

process_t * pm_find_pid(proc_man_t * pm, int pid);

int pm_add_proc(proc_man_t * pm, process_t * proc);
int pm_remove_proc(proc_man_t * pm, int pid);

int pm_resume_process(proc_man_t * pm, int pid, ebus_event_t * event);

// TODO move to scheduler
process_t * pm_get_next(proc_man_t * pm);

int pm_push_event(proc_man_t * pm, ebus_event_t * event);

#endif // KERNEL_PROCESS_MANAGER_H
