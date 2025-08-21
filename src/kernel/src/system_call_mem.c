#include "kernel/system_call_mem.h"

#include <stddef.h>

#include "kernel.h"
#include "libk/defs.h"
#include "memory_alloc.h"
#include "process.h"

int sys_call_mem_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    int res = 0;

    switch (int_no) {
        case SYS_INT_MEM_MALLOC: {
            struct _args {
                size_t size;
            } * args = (struct _args *)args_data;

            process_t * curr_proc = get_current_process();

            return PTR2UINT(memory_alloc(&curr_proc->memory, args->size));
        } break;

        case SYS_INT_MEM_REALLOC: {
            struct _args {
                void * ptr;
                size_t size;
            } * args = (struct _args *)args_data;

            process_t * curr_proc = get_current_process();

            return PTR2UINT(memory_realloc(&curr_proc->memory, args->ptr, args->size));
        } break;

        case SYS_INT_MEM_FREE: {
            struct _args {
                void * ptr;
            } * args = (struct _args *)args_data;

            process_t * curr_proc = get_current_process();

            memory_free(&curr_proc->memory, args->ptr);
        } break;
    }

    return res;
}
