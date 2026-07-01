#include "cpu/mmu.h"

#include "libc/proc.h"
#include "libc/string.h"

void mmu_dir_clear(mmu_dir_t * dir) {
    kmemset(dir, 0, sizeof(mmu_dir_t));
}

void mmu_table_clear(mmu_table_t * table) {
    kmemset(table, 0, sizeof(mmu_table_t));
}

int mmu_dir_set_addr(mmu_dir_t * dir, size_t i, uint32_t addr) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return -1;
    }

    mmu_entry_t entry = dir->entries[i];
    entry &= MASK_FLAGS;
    entry |= addr & MASK_ADDR;

    dir->entries[i] = entry;

    return 0;
}

int mmu_dir_set_flags(mmu_dir_t * dir, size_t i, uint32_t flags) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return -1;
    }

    mmu_entry_t entry = dir->entries[i];
    entry &= MASK_ADDR;
    entry |= flags & MASK_FLAGS;

    dir->entries[i] = entry;

    return 0;
}

int mmu_dir_set(mmu_dir_t * dir, size_t i, uint32_t addr, uint32_t flags) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return -1;
    }

    mmu_entry_t entry = (addr & MASK_ADDR) | (flags & MASK_FLAGS);

    dir->entries[i] = entry;

    return 0;
}

uint32_t mmu_dir_get_addr(mmu_dir_t * dir, size_t i) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return 0;
    }

    return dir->entries[i] & MASK_ADDR;
}

uint32_t mmu_dir_get_flags(mmu_dir_t * dir, size_t i) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return 0;
    }

    return dir->entries[i] & MASK_FLAGS;
}

int mmu_table_set_addr(mmu_table_t * table, size_t i, uint32_t addr) {
    if (!table || i >= MMU_TABLE_SIZE) {
        return -1;
    }

    mmu_entry_t entry = table->entries[i];
    entry &= MASK_FLAGS;
    entry |= addr & MASK_ADDR;

    table->entries[i] = entry;

    return 0;
}

int mmu_table_set_flags(mmu_table_t * table, size_t i, uint32_t flags) {
    if (!table || i >= MMU_TABLE_SIZE) {
        return -1;
    }

    mmu_entry_t entry = table->entries[i];
    entry &= MASK_ADDR;
    entry |= flags & MASK_FLAGS;

    table->entries[i] = entry;

    return 0;
}

int mmu_table_set(mmu_table_t * table, size_t i, uint32_t addr, uint32_t flags) {
    if (!table || i >= MMU_TABLE_SIZE) {
        return -1;
    }

    mmu_entry_t entry = (addr & MASK_ADDR) | (flags & MASK_FLAGS);

    table->entries[i] = entry;

    return 0;
}

uint32_t mmu_table_get_addr(mmu_table_t * table, size_t i) {
    if (!table || i >= MMU_TABLE_SIZE) {
        return 0;
    }

    return table->entries[i] & MASK_ADDR;
}

uint32_t mmu_table_get_flags(mmu_table_t * table, size_t i) {
    if (!table || i >= MMU_TABLE_SIZE) {
        return 0;
    }

    return table->entries[i] & MASK_FLAGS;
}

// GCOVR_EXCL_START
void mmu_flush_tlb(uint32_t vaddr) {
    asm volatile("invlpg (%0)" ::"r"(vaddr) : "memory");
}
// GCOVR_EXCL_STOP
