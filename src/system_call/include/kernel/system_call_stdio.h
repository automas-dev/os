#ifndef KERNEL_SYSTEM_CALL_STDIO_H
#define KERNEL_SYSTEM_CALL_STDIO_H

#include "system_call.h"

int sys_call_tmp_stdio_cb(uint16_t int_no, void * args_data, registers_t * regs);

#endif // KERNEL_SYSTEM_CALL_STDIO_H
