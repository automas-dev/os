#ifndef KERNEL_H
#define KERNEL_H

#include <stddef.h>
#include <stdint.h>

#include "drivers/tar.h"
#include "ebus.h"
#include "kernel/memory.h"
#include "kernel/panic.h"
#include "kernel/scheduler.h"
#include "memory_alloc.h"
#include "process.h"
#include "process_manager.h"

typedef struct _kernel {
    uint32_t    esp0;
    proc_man_t  pm;
    scheduler_t scheduler;
    ebus_t      event_queue;
    disk_t *    disk;
    tar_fs_t *  tar;
} kernel_t;

disk_t *   kernel_get_disk();
tar_fs_t * kernel_get_tar();

kernel_t * get_kernel();

process_t * get_current_process();

ebus_t *     get_kernel_ebus();
proc_man_t * kernel_get_proc_man();
process_t *  kernel_find_pid(int pid);

// Returns pid
int kernel_exec(const char * filename, size_t argc, char ** argv);

void tmp_register_signals_cb(signals_master_cb_t cb);

void kernel_queue_event(ebus_event_t * event);

// ebus_event_t * pull_event(int event_id);

int kernel_close_process(process_t * proc);

typedef int (*_proc_call_t)(void * data);

int kernel_call_as_proc(int pid, _proc_call_t fn, void * data);

int kernel_switch_task();

#endif // KERNEL_H
