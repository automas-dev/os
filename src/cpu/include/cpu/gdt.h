#ifndef CPU_GDT_H
#define CPU_GDT_H

#include "cpu/tss.h"
#include "defs.h"

// https://wiki.osdev.org/Global_Descriptor_Table#Segment_Descriptor

enum GDT_ACCESS {
    GDT_ACCESS_ACCESSED          = 0x1,
    GDT_ACCESS_CODE_READ         = 0x2,
    GDT_ACCESS_DATA_WRITE        = 0x2,
    GDT_ACCESS_DATA_DOWN         = 0x4,
    GDT_ACCESS_CODE_DPL_STRICT   = 0x4,
    GDT_ACCESS_EXECUTABLE        = 0x8,  // 0 = data, 1 = code
    GDT_ACCESS_CODE_DATA_SEGMENT = 0x10, // 0 = system ie tss, 1 = code/data (this)
    GDT_ACCESS_PRESENT           = 0x80,
};

enum GDT_FLAGS {
    // 0x1 is reserved
    GDT_FLAGS_LONG_MODE = 0x2, // 1 = 64 bit (size flag must be 0)
    GDT_FLAGS_SIZE      = 0x4, // 0 = 16 bit 1 = 32 bit
    GDT_FLAGS_GRANULAR  = 0x8, // 0 = 1 byte, 1 = 4 KiB
};

enum GDT_SYSTEM_ACCESS_TYPE {
    GDT_SYSTEM_ACCESS_TYPE_16_TSS_AVAIL = 0x1,
    GDT_SYSTEM_ACCESS_TYPE_LDT          = 0x2,
    GDT_SYSTEM_ACCESS_TYPE_16_TSS_BUSY  = 0x3,
    GDT_SYSTEM_ACCESS_TYPE_32_TSS_AVAIL = 0x4,
    GDT_SYSTEM_ACCESS_TYPE_32_TSS_BUSY  = 0x5,
};

enum GDT_DPL {
    GDT_DPL_RING_0      = 0x0,
    GDT_DPL_RING_KERNEL = 0x0,
    GDT_DPL_RING_1      = 0x1,
    GDT_DPL_RING_2      = 0x2,
    GDT_DPL_RING_3      = 0x3,
    GDT_DPL_RING_USER   = 0x3,
};

#define GDT_DPL_OFFSET 0x5

#define GDT_PRESET_KERNEL_CODE_FLAGS  GDT_FLAGS_SIZE | GDT_FLAGS_GRANULAR
#define GDT_PRESET_KERNEL_CODE_ACCESS GDT_ACCESS_CODE_READ | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_CODE_DATA_SEGMENT | GDT_ACCESS_PRESENT
#define GDT_PRESET_KERNEL_DATA_FLAGS  GDT_FLAGS_SIZE | GDT_FLAGS_GRANULAR
#define GDT_PRESET_KERNEL_DATA_ACCESS GDT_ACCESS_DATA_WRITE | GDT_ACCESS_CODE_DATA_SEGMENT | GDT_ACCESS_PRESENT

#define GDT_PRESET_USER_CODE_FLAGS  GDT_FLAGS_SIZE | GDT_FLAGS_GRANULAR
#define GDT_PRESET_USER_CODE_ACCESS GDT_PRESET_KERNEL_CODE_ACCESS | (GDT_DPL_RING_USER << GDT_DPL_OFFSET)
#define GDT_PRESET_USER_DATA_FLAGS  GDT_FLAGS_SIZE | GDT_FLAGS_GRANULAR
#define GDT_PRESET_USER_DATA_ACCESS GDT_PRESET_KERNEL_DATA_ACCESS | (GDT_DPL_RING_USER << GDT_DPL_OFFSET)

#define GDT_PRESET_KERNEL_TSS_FLAGS  GDT_FLAGS_SIZE
#define GDT_PRESET_KERNEL_TSS_ACCESS GDT_ACCESS_ACCESSED | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_PRESENT

#define GDT_PRESET_USER_TSS_FLAGS  GDT_FLAGS_SIZE
#define GDT_PRESET_USER_TSS_ACCESS GDT_ACCESS_ACCESSED | GDT_ACCESS_EXECUTABLE | GDT_ACCESS_PRESENT | (GDT_DPL_RING_USER << GDT_DPL_OFFSET)

enum GDT_ENTRY_INDEX {
    GDT_ENTRY_INDEX_NULL        = 0x0,
    GDT_ENTRY_INDEX_KERNEL_CODE = 0x1,
    GDT_ENTRY_INDEX_KERNEL_DATA = 0x2,
    GDT_ENTRY_INDEX_USER_CODE   = 0x3,
    GDT_ENTRY_INDEX_USER_DATA   = 0x4,
    GDT_ENTRY_INDEX_KERNEL_TSS  = 0x5,
    GDT_ENTRY_INDEX_USER_TSS    = 0x6,
};

// Segment selector: entry index shifted into place with the requested ring
// (RPL) in the low 2 bits. See https://wiki.osdev.org/Segment_Selector
#define GDT_SELECTOR(INDEX, RPL) (((INDEX) * 8) | (RPL))

#define GDT_SELECTOR_KERNEL_CODE GDT_SELECTOR(GDT_ENTRY_INDEX_KERNEL_CODE, GDT_DPL_RING_KERNEL)
#define GDT_SELECTOR_KERNEL_DATA GDT_SELECTOR(GDT_ENTRY_INDEX_KERNEL_DATA, GDT_DPL_RING_KERNEL)
#define GDT_SELECTOR_USER_CODE   GDT_SELECTOR(GDT_ENTRY_INDEX_USER_CODE, GDT_DPL_RING_USER)
#define GDT_SELECTOR_USER_DATA   GDT_SELECTOR(GDT_ENTRY_INDEX_USER_DATA, GDT_DPL_RING_USER)

typedef struct {
    uint16_t limit_low  : 16;
    uint32_t base_low   : 24;
    uint8_t  access     : 8;
    uint8_t  limit_high : 4;
    uint8_t  flags      : 4;
    uint8_t  base_high  : 8;
} __attribute__((packed)) gdt_entry_t;

void init_gdt();

size_t        gdt_entry_count();
gdt_entry_t * gdt_get_entry(size_t i);

int gdt_set(size_t i, uint64_t base, uint64_t limit, uint8_t access, uint8_t flags);
int gdt_set_base(size_t i, uint64_t base);
int gdt_set_limit(size_t i, uint64_t limit);
int gdt_set_access(size_t i, uint8_t access);
int gdt_set_flags(size_t i, uint8_t flags);

#endif // CPU_GDT_H
