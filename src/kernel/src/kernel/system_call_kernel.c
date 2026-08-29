#define KLOG_SERVICE "KERNEL/SYSTEM_CALL_KERNEL"

#include "kernel/system_call_kernel.h"

#include <stddef.h>

#include "config.h"
#include "defs.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "libc/proc.h"
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
            return (int)(PROJECT_DESCRIPTION);
        } break;
    }

    return 0;
}
