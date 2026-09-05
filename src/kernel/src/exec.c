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

int command_exec(uint8_t * buff, const char * filepath, size_t size, size_t argc, char ** argv) {
    if (!buff) {
        KLOG_WARNING("Tried to execute null buffer");
        return -1;
    }
    if (!filepath) {
        KLOG_WARNING("Missing filepath");
        return -1;
    }
    if (!size) {
        KLOG_WARNING("Buffer size is 0");
    }
    if (argc && !argv) {
        KLOG_WARNING("Missing argv");
        return -1;
    }

    process_t * proc = kmalloc(sizeof(process_t));
    if (!proc) {
        KLOG_ERROR("Failed to malloc process_t");
        return -1;
    }

    process_t * active = get_active_task();

    if (process_create(proc)) {
        KLOG_DEBUG("Failed to create process");
        kfree(proc);
        return -1;
    }

    if (process_load_heap(proc, buff, size)) {
        KLOG_DEBUG("Failed to load heap");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling load heap error");
        }
        kfree(proc);
        return -1;
    }

    if (process_copy_args(proc, filepath, argc, argv)) {
        KLOG_DEBUG("Failed to copy args");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling copy args error");
        }
        kfree(proc);
        return -1;
    }

    KLOG_TRACE("Setting process entrypoint to %p", VADDR_USER_MEM);

    if (process_set_entrypoint(proc, UINT2PTR(VADDR_USER_MEM))) {
        KLOG_DEBUG("Failed to set entrypoint");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling set entrypoint error");
        }
        kfree(proc);
        return -1;
    }

    KLOG_TRACE("Adding 32 pages");

    if (!process_add_pages(proc, 32)) {
        KLOG_DEBUG("Failed to add 32 pages");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling add 32 pages error");
        }
        kfree(proc);
        return -1;
    }

    if (pm_add_proc(kernel_get_proc_man(), proc)) {
        KLOG_DEBUG("Failed to add process to process manager");
        if (process_free(proc)) {
            KPANIC("Failed to free process while handling add process to process manager error");
        }
        kfree(proc);
        return -1;
    }

    if (pm_resume_process(kernel_get_proc_man(), proc->pid)) {
        KLOG_DEBUG("Failed to resume process %u", proc->pid);
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
