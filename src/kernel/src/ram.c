#include "kernel/device/ram.h"

#include "cpu/mmu.h"
#include "libc/string.h"

#define REGION_MAX_PAGE_COUNT 0x8000
#define REGION_MAX_SIZE       (REGION_MAX_PAGE_COUNT * PAGE_SIZE)

#define REGION_TABLE_FLAG_PRESENT 0x1
#define BITMASK_PAGE_FREE         0x1

static ram_table_t * __region_table;
static size_t        __region_table_count;
static void *        __bitmask;

static int  find_addr_entry(uint32_t addr, size_t * out_bit_i);
static int  find_free_bit(const void * bitmask, size_t page_count);
static int  find_free_region();
static void set_bit_used(void * bitmask, size_t bit);
static void set_bit_free(void * bitmask, size_t bit);
static int  is_bit_free(void * bitmask, size_t bit);
static void fill_bitmask(void * bitmask, size_t page_count);
static void add_memory_at(size_t start, uint64_t base, uint64_t length);

int ram_init(ram_table_t * ram_table, void * bitmasks) {
    if (!ram_table || !bitmasks) {
        return -1;
    }

    __region_table       = ram_table;
    __region_table_count = 0;
    __bitmask            = bitmasks;

    kmemset(__region_table, 0, sizeof(ram_table_t));

    return 0;
}

int ram_region_add_memory(uint64_t base, uint64_t length) {
    if (!base || !length || base & 0xfff) {
        return -1;
    }

    if (mmu_paging_enabled()) {
        return -1;
    }

    size_t split_count = length / REGION_MAX_SIZE;

    if (__region_table_count + split_count >= REGION_TABLE_SIZE) {
        return -1;
    }

    if (length < PAGE_SIZE * 2) {
        return -1;
    }

    for (size_t i = 0; i < __region_table_count; i++) {
        ram_table_entry_t * entry = &__region_table->entries[i];

        uint32_t region_start = entry->addr_flags & MASK_ADDR;

        if (region_start == base) {
            return -1;
        }

        if (base < region_start) {
            size_t to_move = (__region_table_count - i) * sizeof(ram_table_entry_t);

            ram_table_entry_t * dest = &__region_table->entries[i + split_count + 1];
            kmemmove(dest, entry, to_move);

            add_memory_at(i, base, length);

            return 0;
        }
    }

    add_memory_at(__region_table_count, base, length);

    return 0;
}

size_t ram_region_table_count() {
    return __region_table_count;
}

size_t ram_free_pages() {
    size_t pages = 0;

    for (size_t i = 0; i < __region_table_count; i++) {
        pages += __region_table->entries[i].free_count;
    }

    return pages;
}

size_t ram_max_pages() {
    size_t pages = 0;

    for (size_t i = 0; i < __region_table_count; i++) {
        pages += __region_table->entries[i].page_count;
    }

    return pages;
}

uint32_t ram_page_alloc() {
    int region_i = find_free_region();

    if (region_i < 0) {
        return 0;
    }

    ram_table_entry_t * entry = &__region_table->entries[region_i];
    // In virtual address space
    void * bitmask = __bitmask + PAGE_SIZE * region_i;
    int    bit_i   = find_free_bit(bitmask, entry->page_count);

    if (bit_i < 0) {
        return 0;
    }

    set_bit_used(bitmask, bit_i);
    entry->free_count--;

    return (entry->addr_flags & MASK_ADDR) + PAGE_SIZE * bit_i;
}

uint32_t ram_page_palloc() {
    if (mmu_paging_enabled()) {
        return 0;
    }

    int region_i = find_free_region();

    if (region_i < 0) {
        return 0;
    }

    ram_table_entry_t * entry = &__region_table->entries[region_i];
    // In physical address space
    void * bitmask = (void *)(entry->addr_flags & MASK_ADDR);
    int    bit_i   = find_free_bit(bitmask, entry->page_count);

    if (bit_i < 0) {
        return 0;
    }

    set_bit_used(bitmask, bit_i);
    entry->free_count--;

    return (entry->addr_flags & MASK_ADDR) + PAGE_SIZE * bit_i;
}

int ram_page_free(uint32_t addr) {
    size_t bit_i    = 0;
    int    region_i = find_addr_entry(addr, &bit_i);

    if (region_i < 0) {
        return -1;
    }

    ram_table_entry_t * entry = &__region_table->entries[region_i];
    // In virtual address space
    void * bitmask = __bitmask + PAGE_SIZE * region_i;

    if (is_bit_free(bitmask, bit_i)) {
        return -1;
    }

    set_bit_free(bitmask, bit_i);
    entry->free_count++;

    return 0;
}

static int find_addr_entry(uint32_t addr, size_t * out_bit_i) {
    for (size_t i = 0; i < __region_table_count; i++) {
        ram_table_entry_t * entry = &__region_table->entries[i];

        uint32_t region_start = entry->addr_flags & MASK_ADDR;
        uint32_t region_end   = region_start + entry->page_count * PAGE_SIZE;

        if (addr >= region_start && addr <= region_end) {
            *out_bit_i = (addr - region_start) / PAGE_SIZE;

            return i;
        }
    }

    return -1;
}

static int find_free_bit(const void * bitmask, size_t page_count) {
    const char * bitmask_data = bitmask;

    for (size_t i = 1; i < page_count; i++) {
        size_t byte = i / 8;
        size_t bit  = i % 8;

        if (bitmask_data[byte] & (1 << bit)) {
            return i;
        }
    }

    return -1;
}

static int find_free_region() {
    for (int i = 0; i < __region_table_count; i++) {
        ram_table_entry_t * entry = &__region_table->entries[i];

        if (entry->free_count) {
            return i;
        }
    }

    return -1;
}

static void set_bit_used(void * bitmask, size_t bit) {
    char * bitmask_data = bitmask;

    size_t byte = bit / 8;
    bit         = bit % 8;

    bitmask_data[byte] &= ~(1 << bit);
}

static void set_bit_free(void * bitmask, size_t bit) {
    char * bitmask_data = bitmask;

    size_t byte = bit / 8;
    bit         = bit % 8;

    bitmask_data[byte] |= 1 << bit;
}

static int is_bit_free(void * bitmask, size_t bit) {
    char * bitmask_data = bitmask;

    size_t byte = bit / 8;
    bit         = bit % 8;

    return bitmask_data[byte] & (1 << bit);
}

/**
 * @brief Fill a bitmask with free bits for `page_count` number of pages.
 *
 * `page_count` includes the bitmask page
 *
 * @param bitmask pointer to the bitmask
 * @param page_count number of pages in region including the bitmask page
 */
static void fill_bitmask(void * bitmask, size_t page_count) {
    unsigned char * bitmask_data = (unsigned char *)bitmask;

    size_t bytes    = page_count / 8;
    size_t end_bits = page_count % 8;

    kmemset(bitmask_data, 0, PAGE_SIZE);
    kmemset(bitmask_data, 0xff, bytes);

    if (end_bits) {
        char last_byte = 0;

        for (size_t bit = 0; bit < end_bits; bit++) {
            last_byte = (last_byte << 1) | 1;
        }

        bitmask_data[bytes] = last_byte;
    }

    bitmask_data[0] &= 0xfe;
}

static void add_memory_at(size_t start, uint64_t base, uint64_t length) {
    size_t split_count = length / REGION_MAX_SIZE;

    for (size_t i = 0; i <= split_count; i++) {
        __region_table_count++;

        size_t page_count = REGION_MAX_SIZE >> 12;

        if (i == split_count) {
            page_count = (length % REGION_MAX_SIZE) >> 12;
        }

        ram_table_entry_t * entry = &__region_table->entries[start + i];
        entry->addr_flags         = base | REGION_TABLE_FLAG_PRESENT;
        entry->page_count         = page_count;
        entry->free_count         = page_count - 1;

        fill_bitmask((void *)(uint32_t)base, page_count);

        base += REGION_MAX_SIZE;
    }
}
