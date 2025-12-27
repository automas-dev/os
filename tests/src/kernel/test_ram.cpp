#include <array>
#include <cstdlib>

#include "test_common.h"

#define PAGE_SIZE 4096

#define REGION_MAX_PAGE_COUNT 0x8000
#define REGION_MAX_SIZE       (REGION_MAX_PAGE_COUNT * PAGE_SIZE)

extern "C" {
#include "drivers/ram.h"

// 1 page for each region in the table + 1 for alignment
#define BITMASK_SIZE (REGION_TABLE_SIZE * PAGE_SIZE + PAGE_SIZE)

// 2 pages for each region in the table + 1 for alignment
#define BUFFER_SIZE (REGION_MAX_SIZE * 2)

std::array<char, BITMASK_SIZE> bitmasks;
std::array<char, BUFFER_SIZE>  buffer;

static ram_table_t ram;
}

TEST(RamStatic, ram_init) {
    init_mocks();

    kmemset_fake.custom_fake = 0;

    EXPECT_NE(0, ram_init(0, 0));
    EXPECT_NE(0, ram_init(0, (void *)1));
    EXPECT_NE(0, ram_init(&ram, 0));

    kmemset_fake.return_val = (void *)1;
    EXPECT_EQ(0, ram_init(&ram, bitmasks.data()));
    EXPECT_EQ(1, kmemset_fake.call_count);
}

struct Region {
    uint64_t addr;
    size_t   index;
    size_t   size;
    char *   data;

    Region() = default;

    Region(void * data, size_t length) {
        addr = (uint64_t)data;

        if (addr & 0xfff) {
            addr = (addr + PAGE_SIZE) & 0xfffff000;
        }

        length &= 0xfffff000;

        if (length > REGION_MAX_SIZE) {
            length = REGION_MAX_SIZE;
        }

        index      = addr - (uint64_t)data;
        size       = length;
        this->data = (char *)addr;
    }
};

class EmptyRam : public ::testing::Test {
protected:
    // 3 pages (one is bitmask)
    Region region_1;

    // Max size
    Region region_2;

    size_t next_page;
    size_t next_table;

    void SetUp() override {
        init_mocks();

        kmemset_fake.custom_fake = 0;

        // Use Region to page align address
        region_1 = Region(buffer.data(), PAGE_SIZE * 3);
        region_2 = Region(buffer.data() + PAGE_SIZE * 3, REGION_MAX_SIZE);

        bitmasks.fill(0);
        buffer.fill(0);

        memset(&ram, 0, sizeof(ram_table_t));

        mmu_paging_enabled_fake.return_val = false;

        ram_init(&ram, bitmasks.data());

        next_page  = 0;
        next_table = 0;

        RESET_FAKE(kmemset);
    }

    char * add_region(size_t pages = 3) {
        char * data = region_1.data + PAGE_SIZE * next_page;

        bool old_return                    = mmu_paging_enabled_fake.return_val;
        mmu_paging_enabled_fake.return_val = false;

        EXPECT_EQ(0, ram_region_add_memory((uint64_t)data, pages * PAGE_SIZE));

        char * src  = (char *)(ram.entries[next_table].addr_flags & 0xfffff000);
        char * dest = (bitmasks.data() + next_table * PAGE_SIZE);
        memcpy(dest, src, PAGE_SIZE);

        mmu_paging_enabled_fake.return_val = old_return;

        next_page += pages;
        next_table++;

        return data;
    }
};

class Ram : public EmptyRam {
protected:
    void SetUp() override {
        EmptyRam::SetUp();

        add_region(3);

        mmu_paging_enabled_fake.return_val = true;
    }
};

// ram_region_add_memory

TEST_F(EmptyRam, ram_region_add_memory_InvalidParameters) {
    EXPECT_NE(0, ram_region_add_memory(0, 0));
    EXPECT_NE(0, ram_region_add_memory(0x1000, 0));
    EXPECT_NE(0, ram_region_add_memory(0, 1));

    // Not aligned
    EXPECT_NE(0, ram_region_add_memory(1, 0));
    EXPECT_NE(0, ram_region_add_memory(1, 1));

    EXPECT_EQ(0, ram_region_table_count());
}

TEST_F(EmptyRam, ram_region_add_memory_PagingEnabled) {
    mmu_paging_enabled_fake.return_val = true;
    EXPECT_NE(0, ram_region_add_memory(region_1.addr, region_1.size));
    EXPECT_EQ(0, ram_region_table_count());
}

TEST_F(EmptyRam, ram_region_add_memory_TooBig) {
    EXPECT_EQ(0, ram_region_add_memory(region_2.addr, REGION_MAX_SIZE + PAGE_SIZE * 2));
    EXPECT_EQ(2, ram_region_table_count());

    EXPECT_EQ(4, kmemset_fake.call_count);
    EXPECT_EQ(0, kmemmove_fake.call_count);
}

TEST_F(EmptyRam, ram_region_add_memory_TooMany) {
    for (size_t i = 0; i < REGION_TABLE_SIZE; i++) {
        EXPECT_EQ(0, ram_region_add_memory(region_1.addr + PAGE_SIZE * 2 * i, PAGE_SIZE * 2));
    }

    EXPECT_NE(0, ram_region_add_memory(region_1.addr + PAGE_SIZE * 2 * REGION_TABLE_SIZE, PAGE_SIZE * 2));
    EXPECT_EQ(512, ram_region_table_count());

    EXPECT_EQ(1024, kmemset_fake.call_count);
    EXPECT_EQ(0, kmemmove_fake.call_count);
}

TEST_F(EmptyRam, ram_region_add_memory_TooSmall) {
    EXPECT_NE(0, ram_region_add_memory(0x1000, 0));
    EXPECT_NE(0, ram_region_add_memory(0x1000, PAGE_SIZE));
    EXPECT_NE(0, ram_region_add_memory(0x1000, PAGE_SIZE * 2 - 1));
    EXPECT_EQ(0, ram_region_table_count());

    EXPECT_EQ(0, kmemset_fake.call_count);
    EXPECT_EQ(0, kmemmove_fake.call_count);
}

TEST_F(EmptyRam, ram_region_add_memory_Single) {
    EXPECT_EQ(0, ram_region_add_memory(region_1.addr, region_1.size));
    EXPECT_EQ(1, ram_region_table_count());

    EXPECT_EQ(0x6, region_1.data[0]);
    EXPECT_EQ(0, region_1.data[1]);

    EXPECT_EQ(region_1.addr | 0x1, ram.entries[0].addr_flags);
    EXPECT_EQ(3, ram.entries[0].page_count);
    EXPECT_EQ(2, ram.entries[0].free_count);

    EXPECT_EQ(0, ram.entries[1].addr_flags);

    EXPECT_EQ(2, kmemset_fake.call_count);

    EXPECT_EQ(region_1.data, kmemset_fake.arg0_history[0]);
    EXPECT_EQ(PAGE_SIZE, kmemset_fake.arg2_history[0]);

    EXPECT_EQ(region_1.data, kmemset_fake.arg0_history[1]);
    EXPECT_EQ(0, kmemset_fake.arg2_history[1]);

    EXPECT_EQ(0, kmemmove_fake.call_count);
}

TEST_F(EmptyRam, ram_region_add_memory_Same) {
    EXPECT_EQ(0, ram_region_add_memory(region_1.addr, region_1.size));
    EXPECT_NE(0, ram_region_add_memory(region_1.addr, region_1.size));
    EXPECT_EQ(1, ram_region_table_count());

    EXPECT_EQ(0x6, region_1.data[0]);
    EXPECT_EQ(0, region_1.data[1]);

    EXPECT_EQ(region_1.addr | 0x1, ram.entries[0].addr_flags);
    EXPECT_EQ(3, ram.entries[0].page_count);
    EXPECT_EQ(2, ram.entries[0].free_count);

    EXPECT_EQ(0, ram.entries[1].addr_flags);

    EXPECT_EQ(2, kmemset_fake.call_count);
    EXPECT_EQ(region_1.data, kmemset_fake.arg0_val);

    EXPECT_EQ(0, kmemmove_fake.call_count);
}

TEST_F(EmptyRam, ram_region_add_memory_Multiple) {
    uint64_t r2 = region_1.addr + PAGE_SIZE * 3;
    uint64_t r3 = region_1.addr + PAGE_SIZE * 6;
    EXPECT_EQ(0, ram_region_add_memory(region_1.addr, PAGE_SIZE * 3));
    EXPECT_EQ(0, ram_region_add_memory(r2, PAGE_SIZE * 3));
    EXPECT_EQ(0, ram_region_add_memory(r3, PAGE_SIZE * 3));
    EXPECT_EQ(3, ram_region_table_count());

    EXPECT_EQ(region_1.addr | 0x1, ram.entries[0].addr_flags);
    EXPECT_EQ(3, ram.entries[0].page_count);
    EXPECT_EQ(2, ram.entries[0].free_count);

    EXPECT_EQ(r2 | 0x1, ram.entries[1].addr_flags);
    EXPECT_EQ(3, ram.entries[1].page_count);
    EXPECT_EQ(2, ram.entries[1].free_count);

    EXPECT_EQ(r3 | 0x1, ram.entries[2].addr_flags);
    EXPECT_EQ(3, ram.entries[2].page_count);
    EXPECT_EQ(2, ram.entries[2].free_count);

    EXPECT_EQ(0, ram.entries[3].addr_flags);

    EXPECT_EQ(6, kmemset_fake.call_count);
    EXPECT_EQ(0, kmemmove_fake.call_count);

    EXPECT_EQ(0x6, region_1.data[0]);
    EXPECT_EQ(0, region_1.data[1]);

    EXPECT_EQ(0x6, region_1.data[PAGE_SIZE * 3 + 0]);
    EXPECT_EQ(0, region_1.data[PAGE_SIZE * 3 + 1]);

    EXPECT_EQ(0x6, region_1.data[PAGE_SIZE * 6 + 0]);
    EXPECT_EQ(0, region_1.data[PAGE_SIZE * 6 + 1]);
}

TEST_F(EmptyRam, ram_region_add_memory_BeforeFirst) {
    uint64_t r2 = region_1.addr + PAGE_SIZE * 3;
    EXPECT_EQ(0, ram_region_add_memory(r2, region_1.size));
    EXPECT_EQ(0, ram_region_add_memory(region_1.addr, region_1.size));
    EXPECT_EQ(2, ram_region_table_count());

    EXPECT_EQ(region_1.addr | 0x1, ram.entries[0].addr_flags);
    EXPECT_EQ(3, ram.entries[0].page_count);
    EXPECT_EQ(2, ram.entries[0].free_count);

    EXPECT_EQ(r2 | 0x1, ram.entries[1].addr_flags);
    EXPECT_EQ(3, ram.entries[1].page_count);
    EXPECT_EQ(2, ram.entries[1].free_count);

    EXPECT_EQ(0, ram.entries[2].addr_flags);

    EXPECT_EQ(4, kmemset_fake.call_count);

    EXPECT_EQ(region_2.data, kmemset_fake.arg0_history[0]);
    EXPECT_EQ(region_2.data, kmemset_fake.arg0_history[1]);
    EXPECT_EQ(PAGE_SIZE, kmemset_fake.arg2_history[0]);
    EXPECT_EQ(0, kmemset_fake.arg2_history[1]);

    EXPECT_EQ(region_1.data, kmemset_fake.arg0_history[2]);
    EXPECT_EQ(region_1.data, kmemset_fake.arg0_history[3]);
    EXPECT_EQ(PAGE_SIZE, kmemset_fake.arg2_history[2]);
    EXPECT_EQ(0, kmemset_fake.arg2_history[3]);

    EXPECT_EQ(1, kmemmove_fake.call_count);
    EXPECT_EQ(&ram.entries[1], kmemmove_fake.arg0_val);
    EXPECT_EQ(&ram.entries[0], kmemmove_fake.arg1_val);
    EXPECT_EQ(sizeof(ram_table_entry_t), kmemmove_fake.arg2_val);
}

// ram_region_table_count

TEST_F(EmptyRam, ram_region_table_count) {
    EXPECT_EQ(0, ram_region_table_count());

    add_region();
    EXPECT_EQ(1, ram_region_table_count());

    add_region();
    EXPECT_EQ(2, ram_region_table_count());
}

// ram_free_pages / ram_max_pages

TEST_F(Ram, ram_free_max_pages) {
    EXPECT_EQ(2, ram_free_pages());
    EXPECT_EQ(3, ram_max_pages());

    add_region();

    EXPECT_EQ(4, ram_free_pages());
    EXPECT_EQ(6, ram_max_pages());

    ram.entries[0].free_count = 1;
    ram.entries[0].page_count = 1;

    EXPECT_EQ(3, ram_free_pages());
    EXPECT_EQ(4, ram_max_pages());

    ram.entries[1].free_count = 1;
    ram.entries[1].page_count = 1;

    EXPECT_EQ(2, ram_free_pages());
    EXPECT_EQ(2, ram_max_pages());
}

// ram_page_alloc

TEST_F(EmptyRam, ram_page_alloc_NoFreeRegion) {
    EXPECT_EQ(0, ram_page_alloc());
}

TEST_F(Ram, ram_page_alloc_NoFreeBit) {
    bitmasks[0] = 0;

    EXPECT_EQ(0, ram_page_alloc());
}

TEST_F(Ram, ram_page_alloc_Single) {
    uint32_t addr = ram_page_alloc();

    EXPECT_EQ(region_1.addr + PAGE_SIZE, addr);

    EXPECT_EQ(0x4, bitmasks[0]);
    EXPECT_EQ(1, ram.entries[0].free_count);
}

TEST_F(Ram, ram_page_alloc_Multiple) {
    EXPECT_EQ(region_1.addr + PAGE_SIZE, ram_page_alloc());
    EXPECT_EQ(region_1.addr + PAGE_SIZE * 2, ram_page_alloc());
    EXPECT_EQ(0, ram_page_alloc());

    EXPECT_EQ(0, bitmasks[0]);
    EXPECT_EQ(0, ram.entries[0].free_count);
}

TEST_F(Ram, ram_page_alloc_AfterFree) {
    ram.entries[0].page_count = 4;
    ram.entries[0].free_count = 3;
    bitmasks[0]               = 0xe;

    ram_page_alloc();
    uint32_t page_2 = ram_page_alloc();
    ram_page_alloc();

    EXPECT_EQ(0, ram.entries[0].free_count);
    EXPECT_EQ(0, bitmasks[0]);

    ram_page_free(page_2);

    EXPECT_EQ(1, ram.entries[0].free_count);
    EXPECT_EQ(0x4, bitmasks[0]);

    EXPECT_EQ(page_2, ram_page_alloc());

    EXPECT_EQ(0, ram.entries[0].free_count);
    EXPECT_EQ(0, bitmasks[0]);
}

// ram_page_palloc

TEST_F(EmptyRam, ram_page_palloc_NoFreeRegion) {
    EXPECT_EQ(0, ram_page_palloc());
}

TEST_F(Ram, ram_page_palloc_NoFreeBit) {
    mmu_paging_enabled_fake.return_val = false;
    region_1.data[0]                   = 0;

    EXPECT_EQ(0, ram_page_palloc());
}

TEST_F(Ram, ram_page_palloc_PagingEnabled) {
    mmu_paging_enabled_fake.return_val = true;

    EXPECT_EQ(0, ram_page_palloc());
}

TEST_F(Ram, ram_page_palloc_Single) {
    mmu_paging_enabled_fake.return_val = false;
    uint32_t addr                      = ram_page_palloc();

    EXPECT_EQ(region_1.addr + PAGE_SIZE, addr);

    EXPECT_EQ(0x4, region_1.data[0]);
    EXPECT_EQ(1, ram.entries[0].free_count);
}

TEST_F(Ram, ram_page_palloc_Multiple) {
    mmu_paging_enabled_fake.return_val = false;
    EXPECT_EQ(region_1.addr + PAGE_SIZE, ram_page_palloc());
    EXPECT_EQ(region_1.addr + PAGE_SIZE * 2, ram_page_palloc());
    EXPECT_EQ(0, ram_page_palloc());

    EXPECT_EQ(0, region_1.data[0]);
    EXPECT_EQ(0, ram.entries[0].free_count);
}

// ram_page_free

TEST_F(Ram, ram_page_free_InvalidParameters) {
    EXPECT_NE(0, ram_page_free(0));
}

TEST_F(Ram, ram_page_free_NotFound) {
    EXPECT_NE(0, ram_page_free(0x1000));
}

TEST_F(Ram, ram_page_free) {
    ram.entries[0].free_count = 1;
    bitmasks[0]               = 0b100;

    EXPECT_EQ(0, ram_page_free(region_1.addr + PAGE_SIZE));

    EXPECT_EQ(0b110, bitmasks[0]);
    EXPECT_EQ(2, ram.entries[0].free_count);
}

TEST_F(Ram, ram_page_free_AlreadyFree) {
    EXPECT_NE(0, ram_page_free(region_1.addr + PAGE_SIZE));

    EXPECT_EQ(0b110, bitmasks[0]);
    EXPECT_EQ(2, ram.entries[0].free_count);
}

TEST_F(Ram, ram_page_free_SecondBit) {
    ram.entries[0].free_count = 1;
    bitmasks[0]               = 0b10;

    EXPECT_EQ(0, ram_page_free(region_1.addr + PAGE_SIZE * 2));

    EXPECT_EQ(0b110, bitmasks[0]);
    EXPECT_EQ(2, ram.entries[0].free_count);
}

TEST_F(Ram, ram_page_free_SecondRegion) {
    add_region();
    ram.entries[1].free_count = 1;
    bitmasks[PAGE_SIZE]       = 0b100;

    EXPECT_EQ(0, ram_page_free(region_1.addr + PAGE_SIZE * 4));

    EXPECT_EQ(0b110, bitmasks[PAGE_SIZE]);
    EXPECT_EQ(2, ram.entries[1].free_count);
}
