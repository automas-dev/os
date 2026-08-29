#include "cpu/gdt.h"

#include "libc/string.h"

extern void load_gdt(uint32_t limit, uint32_t base);

#define GDT_N 7

static gdt_entry_t __gdt[GDT_N];

void init_gdt() {
    kmemset(__gdt, 0, GDT_N * sizeof(gdt_entry_t));

    gdt_set(GDT_ENTRY_INDEX_KERNEL_CODE, 0, 0xfffff, GDT_PRESET_KERNEL_CODE_ACCESS, GDT_PRESET_KERNEL_CODE_FLAGS);
    gdt_set(GDT_ENTRY_INDEX_KERNEL_DATA, 0, 0xfffff, GDT_PRESET_KERNEL_DATA_ACCESS, GDT_PRESET_KERNEL_DATA_FLAGS);
    gdt_set(GDT_ENTRY_INDEX_USER_CODE, 0, 0xfffff, GDT_PRESET_USER_CODE_ACCESS, GDT_PRESET_USER_CODE_FLAGS);
    gdt_set(GDT_ENTRY_INDEX_USER_DATA, 0, 0xfffff, GDT_PRESET_USER_DATA_ACCESS, GDT_PRESET_USER_DATA_FLAGS);
    gdt_set(GDT_ENTRY_INDEX_KERNEL_TSS, 0, 0xfffff, GDT_PRESET_KERNEL_TSS_ACCESS, GDT_PRESET_KERNEL_TSS_FLAGS);
    gdt_set(GDT_ENTRY_INDEX_USER_TSS, 0, 0xfffff, GDT_PRESET_USER_TSS_ACCESS, GDT_PRESET_USER_TSS_FLAGS);

    load_gdt(GDT_N * sizeof(gdt_entry_t) - 1, PTR2UINT(__gdt));
}

size_t gdt_entry_count() {
    return GDT_N;
}

gdt_entry_t * gdt_get_entry(size_t i) {
    if (i >= GDT_N) {
        return 0;
    }

    return &__gdt[i];
}

int gdt_set(size_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t flags) {
    if (i >= GDT_N) {
        return -1;
    }

    gdt_entry_t * gdt_entry = &__gdt[i];
    gdt_entry->limit_low    = limit & 0xffff;
    gdt_entry->base_low     = base & 0xffffff;
    gdt_entry->access       = access;
    gdt_entry->limit_high   = (limit >> 16) & 0xf;
    gdt_entry->flags        = flags;
    gdt_entry->base_high    = (base >> 24) & 0xff;

    return 0;
}

int gdt_set_base(size_t i, uint64_t base) {
    if (i >= GDT_N) {
        return -1;
    }

    gdt_entry_t * gdt_entry = &__gdt[i];
    gdt_entry->base_low     = base & 0xffffff;
    gdt_entry->base_high    = (base >> 24) & 0xff;

    return 0;
}

int gdt_set_limit(size_t i, uint64_t limit) {
    if (i >= GDT_N) {
        return -1;
    }

    gdt_entry_t * gdt_entry = &__gdt[i];
    gdt_entry->limit_low    = limit & 0xffff;
    gdt_entry->limit_high   = (limit >> 16) & 0xf;

    return 0;
}

int gdt_set_access(size_t i, uint8_t access) {
    if (i >= GDT_N) {
        return -1;
    }

    gdt_entry_t * gdt_entry = &__gdt[i];
    gdt_entry->access       = access;

    return 0;
}

int gdt_set_flags(size_t i, uint8_t flags) {
    if (i >= GDT_N) {
        return -1;
    }

    gdt_entry_t * gdt_entry = &__gdt[i];
    gdt_entry->flags        = flags;

    return 0;
}
