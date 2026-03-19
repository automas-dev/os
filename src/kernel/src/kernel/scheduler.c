#define KLOG_SERVICE "SCHEDULER"

#include "kernel/scheduler.h"

#include "ebus.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/string.h"

int scheduler_init(scheduler_t * scheduler, proc_man_t * pm) {
    if (!scheduler) {
        KLOG_ERROR("scheduler_init received a null pointer for the scheduler struct");
        return -1;
    }
    if (!pm) {
        KLOG_ERROR("scheduler_init received a null pointer for the process manager struct");
        return -1;
    }

    KLOG_TRACE("Clearing scheduler struct");
    kmemset(scheduler, 0, sizeof(scheduler_t));

    KLOG_TRACE("Useing process manager %p in scheduler %p", pm, scheduler);
    scheduler->pm = pm;

    KLOG_TRACE("Finished init of scheduler %p", scheduler);

    return 0;
}

int scheduler_run(scheduler_t * scheduler) {
    if (!scheduler) {
        KLOG_ERROR("scheduler_run received a null pointer");
        return -1;
    }

    process_t * proc = get_active_task();
    if (!proc) {
        KPANIC("Failed to get active process");
    }

    process_t * next = pm_get_next(kernel_get_proc_man());
    KLOG_TRACE("Next after %u is %u in state %u", proc->pid, next->pid, next->state);

    if (pm_resume_process(kernel_get_proc_man(), next->pid)) {
        KPANIC("Failed to resume process");
    }

    KLOG_TRACE("Returned to process %u", proc->pid);
}
