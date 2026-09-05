#ifndef CPU_IDT_H
#define CPU_IDT_H

#include <stdint.h>

/* Segment selectors */
#define KERNEL_CS 0x08

/* How every interrupt gate (handler) is defined */
typedef struct {
    uint16_t low_offset; /* Lower 16 bits of handler function address */
    uint16_t sel;        /* Kernel segment selector */
    uint8_t  always0;
    /* First byte
     * Bit 7: "Interrupt is present"
     * Bits 6-5: Privilege level of caller (0=kernel..3=user)
     * Bit 4: Set to 0 for interrupt gates
     * Bits 3-0: bits 1110 = decimal 14 = "32 bit interrupt gate" */
    uint8_t  flags;
    uint16_t high_offset; /* Higher 16 bits of handler function address */
} __attribute__((packed)) idt_gate_t;

#define IDT_FLAG_PRESENT           0x80
#define IDT_FLAG_GATE_32_INTERRUPT 0x0E
#define IDT_FLAG_DPL(RING)         ((RING) << 5)

/* Only ring 0 code may invoke this gate directly with `int` (hardware
 * exceptions/IRQs can still reach it regardless of the current ring) */
#define IDT_FLAG_KERNEL_INTERRUPT_GATE (IDT_FLAG_PRESENT | IDT_FLAG_GATE_32_INTERRUPT)
/* Ring 3 code may also invoke this gate directly with `int` (eg. syscalls) */
#define IDT_FLAG_USER_INTERRUPT_GATE (IDT_FLAG_PRESENT | IDT_FLAG_GATE_32_INTERRUPT | IDT_FLAG_DPL(3))

/* A pointer to the array of interrupt handlers.
 * Assembly instruction 'lidt' will read it */
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

/* Functions implemented in idt.c */
void set_idt_gate(int n, uint32_t handler, uint8_t flags);
void set_idt();

#endif // CPU_IDT_H
