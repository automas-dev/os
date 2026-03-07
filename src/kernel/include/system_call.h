#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H

#include <stdint.h>

#include "cpu/isr.h"
#include "defs.h"

typedef int (*sys_call_handler_t)(uint32_t call_id, void * args_data, registers_t * regs);

void system_call_init(uint8_t isr_interrupt_no);

void system_call_register(uint16_t family, sys_call_handler_t handler);

#endif // SYSTEM_CALL_H
