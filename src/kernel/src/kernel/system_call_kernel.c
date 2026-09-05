#define KLOG_SERVICE "KERNEL/SYSTEM_CALL_KERNEL"

#include "kernel/system_call_kernel.h"

#include <stddef.h>

#include "config.h"
#include "defs.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/proc.h"
#include "libc/string.h"
#include "libk/defs.h"
#include "process.h"

int sys_call_kernel_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc = get_current_process();

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }

        case SYS_CALL_KERNEL_DESCRIBE: {
            // PROJECT_DESCRIPTION is a compile-time string literal baked into
            // the kernel binary's rodata (supervisor-only memory). It must be
            // copied into the calling process' own user-accessible heap
            // before its pointer can be handed back to ring 3 code.
            const char * description = PROJECT_DESCRIPTION;
            void *       user_ptr    = process_copy_to_heap(proc, description, kstrlen(description) + 1);

            if (!user_ptr) {
                KLOG_ERROR("Failed to copy kernel description into process %u heap", proc->pid);
                return 0;
            }

            return PTR2UINT(user_ptr);
        } break;
    }

    return 0;
}
