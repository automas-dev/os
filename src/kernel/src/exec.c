#define KLOG_SERVICE "EXEC"

#include "exec.h"

#include "cpu/mmu.h"
#include "cpu/tss.h"
#include "drivers/ram.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "paging.h"
#include "process.h"

typedef int (*ff_t)(size_t argc, char ** argv);

static void proc_entry();
static int  copy_args(process_t * proc, const char * filepath, int argc, char ** argv);

int command_exec(uint8_t * buff, const char * filepath, size_t size, size_t argc, char ** argv) {
    if (!buff) {
        KLOG_ERROR("Tried to execute null buffer");
        return -1;
    }
    if (!filepath) {
        KLOG_ERROR("Missing filepath");
        return -1;
    }
    if (!size) {
        KLOG_WARNING("Buffer size is 0");
    }
    if (argc && !argv) {
        KLOG_ERROR("Missing argv");
        return -1;
    }

    process_t * proc = kmalloc(sizeof(process_t));
    if (!proc) {
        KLOG_ERROR("Failed to malloc process_t");
        return -1;
    }

    process_t * active = get_active_task();

    if (process_create(proc)) {
        KLOG_ERROR("Failed to create process");
        kfree(proc);
        return -1;
    }

    if (process_load_heap(proc, buff, size)) {
        KLOG_ERROR("Failed to load heap");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling load heap error");
        }
        kfree(proc);
        return -1;
    }

    KLOG_TRACE("Increasing stack by 1022 pages");
    for (size_t i = 0; i < 1022; i++) {
        if (process_grow_stack(proc)) {
            KLOG_ERROR("Failed to add page %u", i);
            if (process_free(proc)) {
                KPANIC("Failed to free process while handling stack grow error");
            }
            kfree(proc);
            return -1;
        }
    }

    if (copy_args(proc, filepath, argc, argv)) {
        KLOG_ERROR("Failed to copy args");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling copy args error");
        }
        kfree(proc);
        return -1;
    }

    KLOG_TRACE("Setting process entrypoint to %p", proc_entry);

    if (process_set_entrypoint(proc, proc_entry)) {
        KLOG_ERROR("Failed to set entrypoint");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling set entrypoint error");
        }
        kfree(proc);
        return -1;
    }

    KLOG_TRACE("Adding 32 pages");

    if (!process_add_pages(proc, 32)) {
        KLOG_ERROR("Failed to add 32 pages");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling add 32 pages error");
        }
        kfree(proc);
        return -1;
    }

    if (pm_add_proc(kernel_get_proc_man(), proc)) {
        KLOG_ERROR("Failed to add process to process manager");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling add process to process manager error");
        }
        kfree(proc);
        return -1;
    }

    if (pm_resume_process(kernel_get_proc_man(), proc->pid)) {
        KLOG_ERROR("Failed to resume process %u", proc->pid);
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling resume error");
        }
        kfree(proc);
        return -1;
    }

    // pm_remove_proc(kernel_get_proc_man(), proc->pid);
    // process_free(proc);

    return proc->pid;
}

static void proc_entry() {
    process_t * proc = get_active_task();
    ff_t        fn   = UINT2PTR(VADDR_USER_MEM);

    // printf("Start task %s with %u args\n", proc->filepath, proc->argc);

    // TODO get start function pointer from elf

    KLOG_TRACE("Entering process pid %u", proc->pid);

    int res           = fn(proc->argc, proc->argv);
    proc->status_code = res;

    KLOG_TRACE("Return from process pid %u with status %d", proc->pid, res);
}

static char * copy_string(const char * str) {
    if (!str) {
        KLOG_ERROR("Tried to copy null string");
        return 0;
    }
    int    len     = kstrlen(str);
    char * new_str = kmalloc(len + 1);
    if (!new_str) {
        KLOG_ERROR("Failed to malloc new string of length %d", len + 1);
        return 0;
    }
    if (!kmemcpy(new_str, str, len + 1)) {
        KLOG_ERROR("Failed to copy %u bytes in memory from %p to %p", len + 1, str, new_str);
        return 0;
    }
    return new_str;
}

static int copy_args(process_t * proc, const char * filepath, int argc, char ** argv) {
    if (!proc) {
        KLOG_ERROR("Tried to copy args for null process");
        return -1;
    }
    if (!filepath) {
        KLOG_ERROR("Missing filepath");
        return -1;
    }
    if (argc && !argv) {
        KLOG_ERROR("Missing argv");
        return -1;
    }

    proc->filepath = copy_string(filepath);
    if (!proc->filepath) {
        KLOG_ERROR("Failed to copy filepath");
        return -1;
    }
    proc->argc = argc + 1;
    proc->argv = kmalloc(sizeof(char *) * (argc + 1));
    if (!proc->argv) {
        KLOG_ERROR("Failed to malloc process_t argv");
        kfree(proc->filepath);
        return -1;
    }

    proc->argv[0] = copy_string(filepath);
    if (!proc->argv[0]) {
        KLOG_ERROR("Failed to copy filepath to argv");
        kfree(proc->argv);
        kfree(proc->filepath);
        return -1;
    }

    for (int i = 0; i < argc; i++) {
        proc->argv[i + 1] = copy_string(argv[i]);
        if (!proc->argv[i + 1]) {
            KLOG_ERROR("Failed to copy arg %d", i);
            for (int j = 0; j < i + 1; j++) {
                kfree(proc->argv[i]);
            }
            kfree(proc->argv);
            kfree(proc->filepath);
            return -1;
        }
    }

    return 0;
}
