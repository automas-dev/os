#ifndef CPU_MMU_H
#define CPU_MMU_H

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"

#define MMU_DIR_SIZE   1024
#define MMU_TABLE_SIZE 1024

#define MMU_DIR_RW   (MMU_DIR_FLAG_PRESENT | MMU_DIR_FLAG_READ_WRITE)
#define MMU_TABLE_RW (MMU_TABLE_FLAG_PRESENT | MMU_TABLE_FLAG_READ_WRITE)

#define MMU_DIR_RW_USER   (MMU_DIR_RW | MMU_DIR_FLAG_USER_SUPERVISOR)
#define MMU_TABLE_RW_USER (MMU_TABLE_RW | MMU_DIR_FLAG_USER_SUPERVISOR)

enum MMU_DIR_FLAG {
    MMU_DIR_FLAG_PRESENT         = 0x1,
    MMU_DIR_FLAG_READ_WRITE      = 0x2,
    MMU_DIR_FLAG_USER_SUPERVISOR = 0x4,
    MMU_DIR_FLAG_WRITE_THROUGH   = 0x8,
    MMU_DIR_FLAG_CACHE_DISABLE   = 0x10,
    MMU_DIR_FLAG_ACCESSED        = 0x20,
    MMU_DIR_FLAG_PAGE_SIZE       = 0x80,
};

enum MMU_TABLE_FLAG {
    MMU_TABLE_FLAG_PRESENT         = 0x1,
    MMU_TABLE_FLAG_READ_WRITE      = 0x2,
    MMU_TABLE_FLAG_USER_SUPERVISOR = 0x4,
    MMU_TABLE_FLAG_WRITE_THROUGH   = 0x8,
    MMU_TABLE_FLAG_CACHE_DISABLE   = 0x10,
    MMU_TABLE_FLAG_ACCESSED        = 0x20,
    MMU_TABLE_FLAG_DIRTY           = 0x40,
    MMU_TABLE_FLAG_PAT             = 0x80,
    MMU_TABLE_FLAG_GLOBAL          = 0x100,
};

typedef uint32_t mmu_entry_t;

typedef struct {
    mmu_entry_t entries[MMU_DIR_SIZE];
} __attribute__((packed)) mmu_dir_t;

typedef struct {
    mmu_entry_t entries[MMU_TABLE_SIZE];
} __attribute__((packed)) mmu_table_t;

void mmu_dir_clear(mmu_dir_t * dir);
void mmu_table_clear(mmu_table_t * table);

int mmu_dir_set_addr(mmu_dir_t * dir, size_t i, uint32_t addr);
int mmu_dir_set_flags(mmu_dir_t * dir, size_t i, uint32_t flags);
int mmu_dir_set(mmu_dir_t * dir, size_t i, uint32_t addr, uint32_t flags);

uint32_t mmu_dir_get_addr(mmu_dir_t * dir, size_t i);
uint32_t mmu_dir_get_flags(mmu_dir_t * dir, size_t i);

int mmu_table_set_addr(mmu_table_t * table, size_t i, uint32_t addr);
int mmu_table_set_flags(mmu_table_t * table, size_t i, uint32_t flags);
int mmu_table_set(mmu_table_t * table, size_t i, uint32_t addr, uint32_t flags);

uint32_t mmu_table_get_addr(mmu_table_t * table, size_t i);
uint32_t mmu_table_get_flags(mmu_table_t * table, size_t i);

void mmu_flush_tlb(uint32_t addr);

extern void     mmu_enable_paging(uint32_t addr);
extern void     mmu_disable_paging(void);
extern bool     mmu_paging_enabled(void);
extern void     mmu_change_dir(uint32_t addr);
extern uint32_t mmu_get_curr_dir(void);
extern void     mmu_reload_dir(void);

#endif // CPU_MMU_H
