#define KLOG_SERVICE "KERNEL/SYSTEM_CALL_MEM"

#include "kernel/system_call_mem.h"

#include <stddef.h>

#include "kernel.h"
#include "kernel/logs.h"
#include "libk/defs.h"
#include "memory_alloc.h"
#include "process.h"

int sys_call_mem_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }
        case SYS_CALL_MEM_MALLOC: {
            struct _args {
                size_t size;
            } * args = (struct _args *)args_data;

            // Lazily initialized: proc->memory's first page must be
            // allocated while this process' own cr3 is loaded (ie. now,
            // since we are handling a syscall trapped from this process),
            // not during process_create (before it has ever run). Checked via
            // `first` rather than `alloc_pages_fn`: memory_init sets
            // alloc_pages_fn unconditionally even if the first page
            // allocation it performs fails, so alloc_pages_fn alone cannot
            // distinguish "never initialized" from "failed to initialize" -
            // retrying with a stale, partially-set memory_t would otherwise
            // reach memory_alloc with a null mem->last and crash the kernel.
            if (!proc->memory.first && process_init_memory(proc)) {
                KLOG_ERROR("Failed to initialize memory allocator for process %u", proc->pid);
                return 0;
            }

            return PTR2UINT(memory_alloc(&proc->memory, args->size));
        } break;

        case SYS_CALL_MEM_REALLOC: {
            struct _args {
                void * ptr;
                size_t size;
            } * args = (struct _args *)args_data;

            return PTR2UINT(memory_realloc(&proc->memory, args->ptr, args->size));
        } break;

        case SYS_CALL_MEM_FREE: {
            struct _args {
                void * ptr;
            } * args = (struct _args *)args_data;

            memory_free(&proc->memory, args->ptr);
        } break;
    }

    return 0;
}
