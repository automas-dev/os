#ifndef SYSTEM_CALL_H
#define SYSTEM_CALL_H

#include <stdint.h>

#include "cpu/isr.h"
#include "defs.h"

typedef int (*sys_call_handler_t)(uint16_t interrupt_no, void * args_data, registers_t * regs);

void init_system_call(uint8_t isr_interrupt_no);

void system_call_register(uint8_t family, sys_call_handler_t handler);

#endif // SYSTEM_CALL_H
