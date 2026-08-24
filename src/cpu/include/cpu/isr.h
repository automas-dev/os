#ifndef CPU_ISR_H
#define CPU_ISR_H

#include <stdint.h>

/* ISRs reserved for CPU exceptions */
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();
/* IRQ definitions */
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void irq16();

#define IRQ0  32 // Programmable Interrupt Timer Interrupt
#define IRQ1  33 // Keyboard Interrupt
#define IRQ2  34 // Cascade (used internally by the two PICs. never raised)
#define IRQ3  35 // COM2 (if enabled)
#define IRQ4  36 // COM1 (if enabled)
#define IRQ5  37 // LPT2 (if enabled)
#define IRQ6  38 // Floppy Disk
#define IRQ7  39 // LPT1 / Unreliable "spurious" interrupt (usually)
#define IRQ8  40 // CMOS real-time clock (if enabled)
#define IRQ9  41 // Free for peripherals / legacy SCSI / NIC
#define IRQ10 42 // Free for peripherals / SCSI / NIC
#define IRQ11 43 // Free for peripherals / SCSI / NIC
#define IRQ12 44 // PS2 Mouse
#define IRQ13 45 // FPU / Coprocessor / Inter-processor
#define IRQ14 46 // Primary ATA Hard Disk
#define IRQ15 47 // Secondary ATA Hard Disk

#define IRQ16 48 // System call

/* Struct which aggregates many registers */
typedef struct {
    uint32_t cr0, cr2, cr3, cr4;
    uint32_t ds;                                     /* Data segment selector */
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* Pushed by pusha. */
    uint32_t int_no, err_code;                       /* Interrupt number and error code (if applicable) */
    uint32_t eip, cs, eflags, useresp, ss;           /* Pushed by the processor automatically */
} registers_t;

void isr_install();
void isr_handler(registers_t r);

extern void           register_kernel_exit(uint32_t eip, uint32_t esp, uint32_t ebp, uint32_t cr3);
extern _Noreturn void kernel_exit();

typedef void (*isr_t)(registers_t *);
void register_interrupt_handler(uint8_t n, isr_t handler);

void print_trace(registers_t * r);

void disable_interrupts();
void enable_interrupts();

#endif // CPU_ISR_H
