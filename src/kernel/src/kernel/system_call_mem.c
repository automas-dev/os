#define KLOG_SERVICE "KERNEL/SYSTEM_CALL_MEM"

#include "kernel/system_call_mem.h"

#include <stddef.h>

#include "kernel.h"
#include "kernel/logs.h"
#include "libk/defs.h"
#include "process.h"

// The heap allocator algorithm itself runs in user space (see
// src/libc/src/memory_alloc.c and src/libc/src/memory.c). This is the only
// memory system call: it simply grows the calling process' own heap.
int sys_call_mem_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }
        case SYS_CALL_MEM_ALLOC_PAGE: {
            struct _args {
                size_t count;
            } * args = (struct _args *)args_data;

            return PTR2UINT(process_add_pages(proc, args->count));
        } break;
    }

    return 0;
}
