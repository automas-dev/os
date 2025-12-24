#include "cpu/tss.h"

#include "cpu/gdt.h"
#include "libc/stdio.h"
#include "libc/string.h"

#define TSS_N 2
static tss_entry_t __tss_stack[TSS_N];

void init_tss() {
    kmemset(__tss_stack, 0, sizeof(__tss_stack));

    gdt_set_base(GDT_ENTRY_INDEX_KERNEL_TSS, PTR2UINT(&__tss_stack[0]));
    gdt_set_base(GDT_ENTRY_INDEX_USER_TSS, PTR2UINT(&__tss_stack[1]));

    tss_set_esp0(VADDR_ISR_STACK);

    flush_tss();
}

tss_entry_t * tss_get_entry(size_t i) {
    if (i >= TSS_N) {
        return 0;
    }

    return &__tss_stack[i];
}

uint32_t tss_get_esp0() {
    return __tss_stack[0].esp0;
}

void tss_set_esp0(uint32_t stack) { // Used when an interrupt occurs
    __tss_stack[0].esp0 = stack;
}
