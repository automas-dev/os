#ifndef KERNEL_SCHEDULER_H
#define KERNEL_SCHEDULER_H

#include "process.h"
#include "process_manager.h"

typedef struct _scheduler {
    proc_man_t * pm;
} scheduler_t;

int scheduler_init(scheduler_t * scheduler, proc_man_t * pm);

int scheduler_run(scheduler_t * scheduler);

/*
Scheduler Functions

scheduler probably won't need to hold much state / data, so idk if there will be
any getter functions.

Do

- Next task (this is where the magic happens, all processes are entered / resumed here)
- Force switch to task? (scheduler should decide this by priority level / priority request)

(Process / process manager need to be able to keep a priority state / level).
*/

#endif // KERNEL_SCHEDULER_H
