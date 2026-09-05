#ifndef CPU_TSS_H
#define CPU_TSS_H

#include <stddef.h>
#include <stdint.h>

typedef struct _tss_entry {
    uint16_t prev_tss;
    uint16_t __res1;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t __res2;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t __res3;
    uint32_t esp3;
    uint16_t ss3;
    uint16_t __res4;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t __res5;
    uint16_t cs;
    uint16_t __res6;
    uint16_t ss;
    uint16_t __res7;
    uint16_t ds;
    uint16_t __res8;
    uint16_t fs;
    uint16_t __res9;
    uint16_t gs;
    uint16_t __res10;
    uint16_t iopb;
    uint16_t __res12;
    uint32_t ssp;
} __attribute__((packed)) tss_entry_t;

extern void flush_tss();

void init_tss();

tss_entry_t * tss_get_entry(size_t i);

uint32_t tss_get_esp0(void);
void     tss_set_esp0(uint32_t stack);

#endif // CPU_TSS_H
