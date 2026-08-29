#include <cstdlib>
#include <cstring>

#include "test_common.h"

extern "C" {
#include "cpu/gdt.h"

FAKE_VOID_FUNC(load_gdt, uint32_t, uint32_t);
}

TEST(GDTStatic, init_gdt) {
    init_gdt();
    EXPECT_EQ(1, load_gdt_fake.call_count);
    EXPECT_EQ(55, load_gdt_fake.arg0_val);
    EXPECT_NE(0, load_gdt_fake.arg1_val);
}

class GDT : public testing::Test {
protected:
    void SetUp() override {
        init_mocks();

        RESET_FAKE(load_gdt);

        init_gdt();
    }
};

// gdt_entry_count

TEST_F(GDT, gdt_entry_count) {
    EXPECT_EQ(7, gdt_entry_count());
}

// gdt_get_entry

TEST_F(GDT, gdt_get_entry_InvalidParameters) {
    EXPECT_EQ(nullptr, gdt_get_entry(7));
}

TEST_F(GDT, gdt_get_entry) {
    EXPECT_NE(nullptr, gdt_get_entry(0));
    EXPECT_NE(nullptr, gdt_get_entry(6));
}

// gdt_set

TEST_F(GDT, gdt_set_InvalidParameters) {
    EXPECT_NE(0, gdt_set(7, 0, 0, 0, 0));
}

TEST_F(GDT, gdt_set) {
    gdt_entry_t * entry = gdt_get_entry(0);

    EXPECT_EQ(0, gdt_set(0, 0x12345678, 0, 0, 0));
    EXPECT_EQ(0x345678, entry->base_low);
    EXPECT_EQ(0x12, entry->base_high);

    EXPECT_EQ(0, gdt_set(0, 0, 0x9abcdef1, 0, 0));
    EXPECT_EQ(0xdef1, entry->limit_low);
    EXPECT_EQ(0xc, entry->limit_high);
}

// gdt_set_base

TEST_F(GDT, gdt_set_base_InvalidParameters) {
    EXPECT_NE(0, gdt_set_base(7, 0));
}

TEST_F(GDT, gdt_set_base) {
    gdt_entry_t * entry = gdt_get_entry(0);

    EXPECT_EQ(0, gdt_set_base(0, 0x12345678));
    EXPECT_EQ(0x345678, entry->base_low);
    EXPECT_EQ(0x12, entry->base_high);
}

// gdt_set_limit

TEST_F(GDT, gdt_set_limit_InvalidParameters) {
    EXPECT_NE(0, gdt_set_limit(7, 0));
}

TEST_F(GDT, gdt_set_limit) {
    gdt_entry_t * entry = gdt_get_entry(0);

    EXPECT_EQ(0, gdt_set_limit(0, 0x12345678));
    EXPECT_EQ(0x5678, entry->limit_low);
    EXPECT_EQ(0x4, entry->limit_high);
}

// gdt_set_access

TEST_F(GDT, gdt_set_access_InvalidParameters) {
    EXPECT_NE(0, gdt_set_access(7, 0));
}

TEST_F(GDT, gdt_set_access) {
    gdt_entry_t * entry = gdt_get_entry(0);

    EXPECT_EQ(0, gdt_set_access(0, 0xff));
    EXPECT_EQ(0xff, entry->access);
}

// gdt_set_flags

TEST_F(GDT, gdt_set_flags_InvalidParameters) {
    EXPECT_NE(0, gdt_set_flags(7, 0));
}

TEST_F(GDT, gdt_set_flags) {
    gdt_entry_t * entry = gdt_get_entry(0);

    EXPECT_EQ(0, gdt_set_flags(0, 0xff));
    EXPECT_EQ(0xf, entry->flags);
}
