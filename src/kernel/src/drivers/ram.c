#include "drivers/ram.h"

#include "cpu/mmu.h"
#include "kernel/logs.h"
#include "libc/string.h"

#undef SERVICE
#define SERVICE "DRIVER/RAM"

#define REGION_MAX_PAGE_COUNT 0x8000
#define REGION_MAX_SIZE       (REGION_MAX_PAGE_COUNT * PAGE_SIZE)

#define REGION_TABLE_FLAG_PRESENT 0x1
#define BITMASK_PAGE_FREE         0x1

static ram_table_t * __region_table;
static size_t        __region_table_count;
static void *        __bitmask;

static int find_addr_entry(uint32_t addr, size_t * out_bit_i);
static int find_free_bit(const void * bitmask, size_t page_count);
static int find_free_region();
static int set_bit_used(void * bitmask, size_t bit);
static int set_bit_free(void * bitmask, size_t bit);
static int is_bit_free(void * bitmask, size_t bit);
static int fill_bitmask(void * bitmask, size_t page_count);
static int add_memory_at(size_t start, uint64_t base, uint64_t length);

int ram_init(ram_table_t * ram_table, void * bitmasks) {
    if (!ram_table) {
        KLOG_ERROR("Tried to initialize with null table");
        return -1;
    }
    if (!bitmasks) {
        KLOG_ERROR("Tried to initialize with null bitmasks");
        return -1;
    }

    __region_table       = ram_table;
    __region_table_count = 0;
    __bitmask            = bitmasks;

    if (!kmemset(__region_table, 0, sizeof(ram_table_t))) {
        KLOG_ERROR("Failed to clear region table");
        return -1;
    }

    KLOG_DEBUG("Initialized driver");

    return 0;
}

int ram_region_add_memory(uint64_t base, uint64_t length) {
    if (!base) {
        KLOG_ERROR("Tried to add region with 0 base");
        return -1;
    }
    if (!length) {
        KLOG_ERROR("Tried to add region with 0 length");
        return -1;
    }
    if (base & 0xfff) {
        KLOG_ERROR("Tried to add region with misaligned base");
        return -1;
    }

    if (mmu_paging_enabled()) {
        KLOG_ERROR("Tried to add region with paging enabled");
        return -1;
    }
    KLOG_DEBUG("Adding ram region base=0x%lX length=0x%lX", base, length);

    size_t split_count = length / REGION_MAX_SIZE;

    if (__region_table_count + split_count >= REGION_TABLE_SIZE) {
        KLOG_ERROR("Split count %u with region table count %u will exceed region table size %u", split_count, __region_table_count, REGION_TABLE_SIZE);
        return -1;
    }

    if (length < PAGE_SIZE * 2) {
        KLOG_ERROR("Region length %lu is less than 2 pages", length);
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
            if (!kmemmove(dest, entry, to_move)) {
                KLOG_ERROR("Failed to move %u bytes in memory from %p to %p", to_move, entry, dest);
                return -1;
            }

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
        KLOG_ERROR("Failed to find region to allocate from");
        return 0;
    }

    ram_table_entry_t * entry = &__region_table->entries[region_i];
    // In virtual address space
    void * bitmask = __bitmask + PAGE_SIZE * region_i;
    int    bit_i   = find_free_bit(bitmask, entry->page_count);

    if (bit_i < 0) {
        KLOG_ERROR("Failed to find page to allocate from region %u", region_i);
        return 0;
    }

    KLOG_TRACE("Allocating new ram page %u in region %d from virtual space", bit_i, region_i);

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
        KLOG_ERROR("Failed to find region to allocate from");
        return 0;
    }

    ram_table_entry_t * entry = &__region_table->entries[region_i];
    // In physical address space
    void * bitmask = (void *)(entry->addr_flags & MASK_ADDR);
    int    bit_i   = find_free_bit(bitmask, entry->page_count);

    if (bit_i < 0) {
        KLOG_ERROR("Failed to find page to allocate from region %u", region_i);
        return 0;
    }

    set_bit_used(bitmask, bit_i);
    entry->free_count--;

    KLOG_TRACE("Allocating new ram page %d in region %d from physical space, region free count is now %u of %u", bit_i, region_i, entry->free_count, entry->page_count);

    return (entry->addr_flags & MASK_ADDR) + PAGE_SIZE * bit_i;
}

int ram_page_free(uint32_t addr) {
    // TODO added this later but didn't check if this case is valid
    if (!addr) {
        KLOG_ERROR("Tried to free address 0");
        return -1;
    }

    size_t bit_i    = 0;
    int    region_i = find_addr_entry(addr, &bit_i);

    if (region_i < 0) {
        KLOG_ERROR("Failed to find region for address 0x%X to free", addr);
        return -1;
    }

    ram_table_entry_t * entry = &__region_table->entries[region_i];
    // In virtual address space
    void * bitmask = __bitmask + PAGE_SIZE * region_i;

    if (is_bit_free(bitmask, bit_i)) {
        KLOG_ERROR("Page %u in region %d for address 0x%X is already free", bit_i, region_i, addr);
        return -1;
    }

    set_bit_free(bitmask, bit_i);
    entry->free_count++;

    KLOG_TRACE("Free ram page %u in region %d with address 0x%X from virtual space, region free count is now %u of %u", bit_i, region_i, addr, entry->free_count, entry->page_count);

    return 0;
}

static int find_addr_entry(uint32_t addr, size_t * out_bit_i) {
    if (!addr) {
        KLOG_ERROR("Searching for null address");
        return -1;
    }
    if (!out_bit_i) {
        KLOG_ERROR("Output pointer is null");
        return -1;
    }
    for (size_t i = 0; i < __region_table_count; i++) {
        ram_table_entry_t * entry = &__region_table->entries[i];

        uint32_t region_start = entry->addr_flags & MASK_ADDR;
        uint32_t region_end   = region_start + entry->page_count * PAGE_SIZE;

        if (addr >= region_start && addr <= region_end) {
            *out_bit_i = (addr - region_start) / PAGE_SIZE;

            return i;
        }
    }

    KLOG_ERROR("Failed to find entry for address %p", addr);

    return -1;
}

static int find_free_bit(const void * bitmask, size_t page_count) {
    if (!bitmask) {
        KLOG_ERROR("Tried to find bitmask of null pointer");
        return -1;
    }
    if (!page_count) {
        KLOG_WARNING("Trying to find free bit form 0 pages");
        return -1;
    }
    const char * bitmask_data = bitmask;

    for (size_t i = 1; i < page_count; i++) {
        size_t byte = i / 8;
        size_t bit  = i % 8;

        if (bitmask_data[byte] & (1 << bit)) {
            return i;
        }
    }

    KLOG_ERROR("Failed to find free bit in bitmask %p", bitmask);

    return -1;
}

static int find_free_region() {
    for (int i = 0; i < __region_table_count; i++) {
        ram_table_entry_t * entry = &__region_table->entries[i];

        if (entry->free_count) {
            return i;
        }
    }

    KLOG_ERROR("Failed to find free region");

    return -1;
}

static int set_bit_used(void * bitmask, size_t bit) {
    if (!bitmask) {
        KLOG_ERROR("Tried to set used bit of null bitmask");
        return -1;
    }

    char * bitmask_data = bitmask;

    size_t byte = bit / 8;
    bit         = bit % 8;

    bitmask_data[byte] &= ~(1 << bit);

    return 0;
}

static int set_bit_free(void * bitmask, size_t bit) {
    if (!bitmask) {
        KLOG_ERROR("Tried to set free bit of null bitmask");
        return -1;
    }

    char * bitmask_data = bitmask;

    size_t byte = bit / 8;
    bit         = bit % 8;

    bitmask_data[byte] |= 1 << bit;

    return 0;
}

static int is_bit_free(void * bitmask, size_t bit) {
    if (!bitmask) {
        KLOG_ERROR("Tried to check if bit is used of null bitmask");
        return 0;
    }

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
static int fill_bitmask(void * bitmask, size_t page_count) {
    if (!bitmask) {
        KLOG_ERROR("Trying to fill bitmask of null pointer");
        return -1;
    }
    if (!page_count) {
        KLOG_WARNING("Trying to fill bitmask for 0 pages");
        return -1;
    }

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

    return 0;
}

static int add_memory_at(size_t start, uint64_t base, uint64_t length) {
    // TODO what are invalid inputs?
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

    return 0;
}
