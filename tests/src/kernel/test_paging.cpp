#include <cstdlib>

#include "test_common.h"

extern "C" {
#include "paging.h"

mmu_table_t table;
mmu_dir_t   dir;

int custom_mmu_dir_set(mmu_dir_t * dir, size_t i, uint32_t addr, uint32_t flags) {
    if (!dir || i >= MMU_DIR_SIZE) {
        return -1;
    }

    mmu_entry_t entry = (addr & MASK_ADDR) | (flags & MASK_FLAGS);

    dir->entries[i] = entry;

    return 0;
}
}

#define EXPECT_BALANCED() EXPECT_EQ(25, paging_temp_available())

TEST(PagingStatic, paging_init) {
    init_mocks();

    paging_init();

    EXPECT_EQ(1, kmemset_fake.call_count);
    EXPECT_NE(nullptr, kmemset_fake.arg0_val);
    EXPECT_EQ(0, kmemset_fake.arg1_val);
    EXPECT_EQ(200, kmemset_fake.arg2_val);

    EXPECT_EQ(25, paging_temp_available());
}

TEST(PagingStatic, paging_init_FailClear) {
    init_mocks();
    kmemset_fake.custom_fake = 0;
    kmemset_fake.return_val  = 0;

    EXPECT_NE(0, paging_init());
}

class Paging : public ::testing::Test {
protected:
    void SetUp() override {
        init_mocks();

        memset(&table, 0, sizeof(mmu_table_t));
        memset(&dir, 0, sizeof(mmu_dir_t));

        paging_init();
    }

    void fill_temp_pages() {
        size_t i = 1;
        while (paging_temp_available()) {
            paging_temp_map(i++ << 12);
        }
    }

    void clear_temp_pages() {
        paging_init();
    }
};

TEST_F(Paging, paging_temp_map_calls_flush_tlb) {
    void * ptr = paging_temp_map(0x1000);
    EXPECT_NE(nullptr, ptr);
    EXPECT_EQ(24, paging_temp_available());
    EXPECT_EQ(1, mmu_flush_tlb_fake.call_count);
    EXPECT_EQ((uint32_t)ptr, mmu_flush_tlb_fake.arg0_val);
}

TEST_F(Paging, paging_temp_map_FailSet) {
    mmu_table_set_fake.return_val = -1;

    EXPECT_EQ(nullptr, paging_temp_map(0x1000));
    EXPECT_EQ(24, paging_temp_available());
}

TEST_F(Paging, paging_temp_map) {
    EXPECT_EQ(nullptr, paging_temp_map(0));
    EXPECT_EQ(nullptr, paging_temp_map(1));

    EXPECT_NE(nullptr, paging_temp_map(0x1000));
    EXPECT_EQ(24, paging_temp_available());

    EXPECT_EQ(0, mmu_get_curr_dir_fake.call_count);

    EXPECT_EQ(1, mmu_table_set_fake.call_count);
    EXPECT_EQ((mmu_table_t *)VADDR_KERNEL_TABLE, mmu_table_set_fake.arg0_val);
    EXPECT_EQ(ADDR2PAGE(VADDR_TMP_PAGE), mmu_table_set_fake.arg1_val);
    EXPECT_EQ(0x1000, mmu_table_set_fake.arg2_val);
    EXPECT_EQ(MMU_TABLE_RW, mmu_table_set_fake.arg3_val);

    for (size_t i = 1; i < 25; i++) {
        EXPECT_NE(nullptr, paging_temp_map((i + 1) << 12)) << i;
    }

    EXPECT_EQ(nullptr, paging_temp_map(0x27000));

    SetUp();

    void * a = paging_temp_map(0x1000);
    EXPECT_NE(nullptr, a);
    EXPECT_EQ(24, paging_temp_available());

    void * b = paging_temp_map(0x1000);
    EXPECT_EQ(a, b);
    EXPECT_EQ(24, paging_temp_available());

    EXPECT_EQ(1, mmu_table_set_fake.call_count);

    paging_temp_free(0x1000);
    EXPECT_EQ(24, paging_temp_available());

    paging_temp_free(0x1000);
    EXPECT_EQ(25, paging_temp_available());
}

TEST_F(Paging, paging_temp_free) {
    paging_temp_map(0x1000);

    init_mocks();

    paging_temp_free(0x2000);
    EXPECT_EQ(24, paging_temp_available());

    // Invalid Parameters
    paging_temp_free(0);
    paging_temp_free(1); // misaligned
    EXPECT_EQ(24, paging_temp_available());

    paging_temp_free(0x1000);
    EXPECT_EQ(25, paging_temp_available());
    EXPECT_EQ(0, mmu_table_set_fake.call_count);

    // Already Freed
    paging_temp_free(0x1000);
    EXPECT_EQ(25, paging_temp_available());
}

TEST_F(Paging, paging_temp_available) {
    EXPECT_EQ(25, paging_temp_available());

    paging_temp_map(0x1000);
    EXPECT_EQ(24, paging_temp_available());

    paging_temp_free(0x1000);
    EXPECT_EQ(25, paging_temp_available());

    for (size_t i = 0; i < 25; i++) {
        EXPECT_NE(nullptr, paging_temp_map((i + 1) << 12)) << i;
    }

    EXPECT_EQ(0, paging_temp_available());
}

TEST_F(Paging, paging_id_map_range) {
    EXPECT_NE(0, paging_id_map_range(MMU_TABLE_SIZE, MMU_TABLE_SIZE));

    EXPECT_EQ(0, paging_id_map_range(0, 0));
    EXPECT_EQ(0, paging_id_map_range(1, 2));
}

TEST_F(Paging, paging_id_map_page) {
    // Invalid parameters
    EXPECT_NE(0, paging_id_map_page(MMU_TABLE_SIZE));

    // success
    EXPECT_EQ(0, paging_id_map_page(1));
    EXPECT_EQ(1, mmu_table_set_fake.call_count);
}

// Paging Add Page

TEST_F(Paging, paging_add_pages_InvalidParameters) {
    EXPECT_NE(0, paging_add_pages(0, 1, 2));

    // Start is after end
    EXPECT_NE(0, paging_add_pages(&dir, 2, 1));

    // End past dir
    EXPECT_NE(0, paging_add_pages(&dir, 1, MMU_DIR_SIZE * MMU_TABLE_SIZE));
}

TEST_F(Paging, paging_add_pages_FailAllocPage) {
    EXPECT_NE(0, paging_add_pages(&dir, 1, 2));
    EXPECT_BALANCED();
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(1);
}

TEST_F(Paging, paging_add_pages_NeedsTable_FailAddTable) {
    uint32_t page_seq[2] = {0x2000, 0};
    SET_RETURN_SEQ(ram_page_alloc, page_seq, 2);

    EXPECT_NE(0, paging_add_pages(&dir, 1, 2));
    EXPECT_EQ(2, ram_page_alloc_fake.call_count);
    EXPECT_BALANCED();
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(1);
}

TEST_F(Paging, paging_add_pages_NeedsTable_FailTempMap) {
    ram_page_alloc_fake.return_val = 0x2000;

    EXPECT_NE(0, paging_add_pages(&dir, 1, 2));
    EXPECT_EQ(2, ram_page_alloc_fake.call_count);
    EXPECT_BALANCED();
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(1); // Table is not freed
}

TEST_F(Paging, paging_add_pages_NeedsTable) {
    mmu_dir_set_fake.custom_fake = custom_mmu_dir_set;

    mmu_dir_get_addr_fake.return_val = 0x1000;
    ram_page_alloc_fake.return_val   = 0x2000;

    EXPECT_EQ(0, paging_add_pages(&dir, 1, 2));

    EXPECT_EQ(0x2003, dir.entries[0]);

    EXPECT_BALANCED();
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(4);
}

TEST_F(Paging, paging_add_pages_HasTable) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0x1000;
    ram_page_alloc_fake.return_val    = 0x2000;

    EXPECT_EQ(0, paging_add_pages(&dir, 1, 2));

    EXPECT_EQ(2, ram_page_alloc_fake.call_count);
    EXPECT_EQ(3, mmu_table_set_fake.call_count); // Include call to paging_temp_free

    EXPECT_EQ(1, mmu_table_set_fake.arg1_history[1]);
    EXPECT_EQ(2, mmu_table_set_fake.arg1_history[2]);
    EXPECT_EQ(0x2000, mmu_table_set_fake.arg2_history[1]);
    EXPECT_EQ(0x2000, mmu_table_set_fake.arg2_history[2]);
    EXPECT_EQ(0x3, mmu_table_set_fake.arg3_history[1]);
    EXPECT_EQ(0x3, mmu_table_set_fake.arg3_history[2]);

    EXPECT_BALANCED();
    ASSERT_RAM_ALLOC_BALANCE_OFFSET(2);
}

// Paging Remove Page

TEST_F(Paging, paging_remove_pages_InvalidParameters) {
    EXPECT_NE(0, paging_remove_pages(0, 1, 2));

    // Start past end
    EXPECT_NE(0, paging_remove_pages(&dir, 2, 1));

    // End Past Dir
    EXPECT_NE(0, paging_remove_pages(&dir, 1, MMU_DIR_SIZE * MMU_TABLE_SIZE));
}

TEST_F(Paging, paging_remove_pages_NoTable) {
    EXPECT_EQ(0, paging_remove_pages(&dir, 1, 2));
    EXPECT_EQ(0, ram_page_free_fake.call_count);
}

TEST_F(Paging, paging_remove_pages_FailTempMap) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;

    EXPECT_NE(0, paging_remove_pages(&dir, 1, 2));
    EXPECT_EQ(0, ram_page_free_fake.call_count);
    EXPECT_BALANCED();
}

TEST_F(Paging, paging_remove_pages_NoPages) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0x1000;

    EXPECT_EQ(0, paging_remove_pages(&dir, 1, 2));
    EXPECT_EQ(0, ram_page_free_fake.call_count);
    EXPECT_BALANCED();
}

TEST_F(Paging, paging_remove_pages) {
    mmu_dir_get_flags_fake.return_val   = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val    = 0x1000;
    mmu_table_get_flags_fake.return_val = MMU_TABLE_FLAG_PRESENT;

    EXPECT_EQ(0, paging_remove_pages(&dir, 1, 2));
    EXPECT_EQ(2, ram_page_free_fake.call_count);
    EXPECT_EQ(3, mmu_flush_tlb_fake.call_count); // +1 for paging_table_map call
    EXPECT_BALANCED();
}

// Paging Add Table

TEST_F(Paging, paging_add_table_InvalidParameters) {
    EXPECT_NE(0, paging_add_table(0, 0));
    EXPECT_NE(0, paging_add_table(&dir, MMU_DIR_SIZE));
}

TEST_F(Paging, paging_add_table_FailPageAlloc) {
    EXPECT_NE(0, paging_add_table(&dir, 1));
}

TEST_F(Paging, paging_add_table_FailMapTable) {
    ram_page_alloc_fake.return_val = 0x1001;

    EXPECT_NE(0, paging_add_table(&dir, 1));
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    ASSERT_RAM_ALLOC_BALANCED();
}

TEST_F(Paging, paging_add_table_HasTable) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    ram_page_alloc_fake.return_val    = 0x1000;

    EXPECT_EQ(0, paging_add_table(&dir, 1));
    EXPECT_EQ(0, ram_page_alloc_fake.call_count);
    ASSERT_RAM_ALLOC_BALANCED();
}

TEST_F(Paging, paging_add_table_NeedsTable) {
    ram_page_alloc_fake.return_val = 0x1000;

    EXPECT_EQ(0, paging_add_table(&dir, 1));
    EXPECT_EQ(1, mmu_table_set_fake.call_count);
    EXPECT_EQ(0x1000, mmu_table_set_fake.arg2_val);
    EXPECT_EQ(1, mmu_dir_set_fake.call_count);
    EXPECT_EQ(&dir, mmu_dir_set_fake.arg0_val);
    EXPECT_EQ(1, mmu_dir_set_fake.arg1_val);
    EXPECT_EQ(0x1000, mmu_dir_set_fake.arg2_val);
    EXPECT_EQ(0x3, mmu_dir_set_fake.arg3_val);
}

// Paging Remove Table

TEST_F(Paging, paging_remove_table_InvalidParameters) {
    EXPECT_NE(0, paging_remove_table(0, 0));
    EXPECT_NE(0, paging_remove_table(&dir, MMU_DIR_SIZE));
}

TEST_F(Paging, paging_remove_table_HasTable) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0x1000;

    EXPECT_EQ(0, paging_remove_table(&dir, 1));
    EXPECT_EQ(1, ram_page_free_fake.call_count);
    EXPECT_EQ(0x1000, ram_page_free_fake.arg0_val);
}

TEST_F(Paging, paging_remove_table_NoTable) {
    EXPECT_EQ(0, paging_remove_table(&dir, 1));
    EXPECT_EQ(0, ram_page_free_fake.call_count);
}

TEST_F(Paging, paging_add_pages_FailSetPage) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0x1000;
    ram_page_alloc_fake.return_val    = 0x2000;
    mmu_table_set_fake.return_val     = -1;

    EXPECT_NE(0, paging_add_pages(&dir, 1, 1));
}

TEST_F(Paging, paging_remove_pages_FailSetPage) {
    mmu_dir_get_flags_fake.return_val   = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val    = 0x1000;
    mmu_table_get_flags_fake.return_val = MMU_TABLE_FLAG_PRESENT;
    mmu_table_get_addr_fake.return_val  = 0x2000;
    mmu_table_set_fake.return_val       = -1;

    EXPECT_NE(0, paging_remove_pages(&dir, 1, 1));
}

TEST_F(Paging, paging_remove_pages_FailFreePage) {
    mmu_dir_get_flags_fake.return_val   = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val    = 0x1000;
    mmu_table_get_flags_fake.return_val = MMU_TABLE_FLAG_PRESENT;
    mmu_table_get_addr_fake.return_val  = 0x2000;
    ram_page_free_fake.return_val       = -1;

    EXPECT_NE(0, paging_remove_pages(&dir, 1, 1));
}

TEST_F(Paging, paging_add_table_FailSet) {
    ram_page_alloc_fake.return_val = 0x1000;
    mmu_dir_set_fake.return_val    = -1;

    EXPECT_NE(0, paging_add_table(&dir, 1));
}

TEST_F(Paging, paging_remove_table_FailGetAddress) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0;

    EXPECT_NE(0, paging_remove_table(&dir, 1));
}

TEST_F(Paging, paging_remove_table_FailSet) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0x1000;
    mmu_dir_set_fake.return_val       = -1;

    EXPECT_NE(0, paging_remove_table(&dir, 1));
}

TEST_F(Paging, paging_remove_table_FailFree) {
    mmu_dir_get_flags_fake.return_val = MMU_DIR_FLAG_PRESENT;
    mmu_dir_get_addr_fake.return_val  = 0x1000;
    ram_page_free_fake.return_val     = -1;

    EXPECT_EQ(0, paging_remove_table(&dir, 1));
}
