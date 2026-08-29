#ifndef KERNEL_SYSTEM_CALL_KERNEL_H
#define KERNEL_SYSTEM_CALL_KERNEL_H

#include "system_call.h"

int sys_call_kernel_cb(uint32_t call_id, void * args_data, registers_t * regs);

#endif // KERNEL_SYSTEM_CALL_KERNEL_H
