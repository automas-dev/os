#include "cpu/idt.h"

#define low_16(address)  (uint16_t)((address) & 0xFFFF)
#define high_16(address) (uint16_t)(((address) >> 16) & 0xFFFF)

#define IDT_ENTRIES 256
static idt_gate_t     __idt[IDT_ENTRIES];
static idt_register_t __idt_reg;

void set_idt_gate(int n, uint32_t handler, uint8_t flags) {
    __idt[n].low_offset  = low_16(handler);
    __idt[n].sel         = KERNEL_CS;
    __idt[n].always0     = 0;
    __idt[n].flags       = flags;
    __idt[n].high_offset = high_16(handler);
}

void set_idt() {
    __idt_reg.base  = (uint32_t)&__idt;
    __idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
    /* Don't make the mistake of loading &__idt -- always load &__idt_reg */
    asm volatile("lidtl (%0)" : : "r"(&__idt_reg));
}
