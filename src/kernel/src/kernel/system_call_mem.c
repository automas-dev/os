#include "kernel/system_call_mem.h"

#include <stddef.h>

#include "kernel.h"
#include "kernel/logs.h"
#include "libk/defs.h"
#include "memory_alloc.h"
#include "process.h"

#undef SERVICE
#define SERVICE "SYSCALL/MEMORY"

int sys_call_mem_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call from pid %u interrupt number 0x%X", proc->pid, int_no);

    switch (int_no) {
        default: {
            KLOG_WARNING("Invalid interrupt number 0x%X", int_no);
            break;
        }
        case SYS_INT_MEM_MALLOC: {
            struct _args {
                size_t size;
            } * args = (struct _args *)args_data;

            return PTR2UINT(memory_alloc(&proc->memory, args->size));
        } break;

        case SYS_INT_MEM_REALLOC: {
            struct _args {
                void * ptr;
                size_t size;
            } * args = (struct _args *)args_data;

            return PTR2UINT(memory_realloc(&proc->memory, args->ptr, args->size));
        } break;

        case SYS_INT_MEM_FREE: {
            struct _args {
                void * ptr;
            } * args = (struct _args *)args_data;

            memory_free(&proc->memory, args->ptr);
        } break;
    }

    return 0;
}
