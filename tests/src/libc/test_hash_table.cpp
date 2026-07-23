#include <cstdlib>
#include <vector>

#include "test_common.h"

extern "C" {
#include <string.h>

#include "libc/datastruct/hash_table.h"
}

class Table : public testing::Test {
protected:
    htable_t table;

    void SetUp() override {
        init_mocks();

        htable_create(&table, 100);

        RESET_FAKE(pmalloc);
        pmalloc_fake.custom_fake = malloc;
    }

    void TearDown() override {
        htable_free(&table);
    }
};

TEST_F(Table, htable_create) {
    // Invalid Parameters
    EXPECT_NE(0, htable_create(&table, 0));
    EXPECT_NE(0, htable_create(0, 1));
    EXPECT_NE(0, htable_create(0, 0));
    EXPECT_EQ(0, pmalloc_fake.call_count);
    EXPECT_EQ(0, pfree_fake.call_count);

    htable_free(&table);
    // Good args
    EXPECT_EQ(0, htable_create(&table, 4));

    ASSERT_EQ(1, pmalloc_fake.call_count);
    EXPECT_EQ(4, pmalloc_fake.arg0_val);

    ASSERT_EQ(1, pmalloc_fake.call_count);
    EXPECT_EQ(8, pmalloc_fake.arg0_val);

    SetUp();

    pmalloc_fake.custom_fake = 0;
    pmalloc_fake.return_val  = 0;

    // First malloc fails
    EXPECT_NE(0, htable_create(&table, 4));
    EXPECT_EQ(1, pmalloc_fake.call_count);
    EXPECT_EQ(4, pmalloc_fake.arg0_val);
}

// TEST_F(Table, arr_free) {
//     // Invalid parameters
//     arr_free(0);
//     EXPECT_EQ(0, pfree_fake.call_count);

//     void * data = arr.data;
//     arr_free(&arr);
//     EXPECT_EQ(1, pfree_fake.call_count);
//     EXPECT_EQ(data, pfree_fake.arg0_val);
//     EXPECT_EQ(0, arr.data);
// }

// TEST_F(Table, arr_size) {
//     // Empty
//     EXPECT_EQ(0, arr_size(&arr));

//     // 1 element
//     char c = '1';
//     EXPECT_EQ(0, arr_insert(&arr, 0, &c));
//     EXPECT_EQ(1, arr_size(&arr));

//     // 2 element
//     c = '2';
//     EXPECT_EQ(0, arr_insert(&arr, 1, &c));
//     EXPECT_EQ(2, arr_size(&arr));

//     // 3 element
//     c = '3';
//     EXPECT_EQ(0, arr_insert(&arr, 2, &c));
//     EXPECT_EQ(3, arr_size(&arr));

//     void * old_data = arr_data(&arr);

//     // Grow buffer
//     c = '4';
//     EXPECT_EQ(0, arr_insert(&arr, 3, &c));
//     EXPECT_EQ(4, arr_size(&arr));

//     ASSERT_EQ(1, prealloc_fake.call_count);
//     EXPECT_EQ(old_data, prealloc_fake.arg0_val);
//     EXPECT_EQ(4, prealloc_fake.arg1_val);
// }

// TEST_F(Table, arr_data) {
//     EXPECT_NE(nullptr, arr_data(&arr));
// }

// TEST_F(Table, arr_at) {
//     EXPECT_EQ(0, arr_at(&arr, 0));
//     EXPECT_EQ(0, arr_at(&arr, 1));
//     EXPECT_EQ(0, arr_at(&arr, 2));
//     EXPECT_EQ(0, arr_at(&arr, 3));

//     char a = '1';
//     EXPECT_EQ(0, arr_insert(&arr, 0, &a));

//     char   b = '1';
//     void * c = arr_at(&arr, 0);
//     EXPECT_NE(0, b);
//     EXPECT_EQ('1', *((char *)c));
//     EXPECT_EQ(0, arr_at(&arr, 1));
//     EXPECT_EQ(0, arr_at(&arr, 2));

//     arr_free(&arr);

//     EXPECT_EQ(0, arr_create(&arr, 2, 4));
//     EXPECT_EQ(0, arr_size(&arr));

//     // No elements
//     EXPECT_EQ(0, arr_at(&arr, 0));
//     EXPECT_EQ(0, arr_at(&arr, 1));

//     // Read past end
//     EXPECT_EQ(0, arr_at(&arr, 2));

//     int i1 = 12345;
//     ASSERT_EQ(0, arr_insert(&arr, 0, &i1));

//     // 1 element
//     int * i2 = (int *)arr_at(&arr, 0);
//     ASSERT_NE(nullptr, i2);
//     EXPECT_EQ(i1, *i2);
//     EXPECT_EQ(0, arr_at(&arr, 1));

//     int i3 = 6789;
//     ASSERT_EQ(0, arr_insert(&arr, 1, &i3));

//     // 2 element
//     i2 = (int *)arr_at(&arr, 0);
//     ASSERT_NE(nullptr, i2);
//     EXPECT_EQ(i1, *i2);

//     i2 = (int *)arr_at(&arr, 1);
//     ASSERT_NE(nullptr, i2);
//     EXPECT_EQ(i3, *i2);
// }

// TEST_F(Table, arr_set) {
//     char c;

//     EXPECT_NE(0, arr_set(0, 0, 0));
//     EXPECT_NE(0, arr_set(&arr, 0, 0));
//     EXPECT_NE(0, arr_set(0, 0, &c));

//     c = 'a';
//     arr_insert(&arr, 0, &c);
//     arr_insert(&arr, 0, &c);
//     arr_insert(&arr, 0, &c);

//     c = 'b';
//     EXPECT_EQ(0, arr_set(&arr, 0, &c));
//     EXPECT_EQ('b', *(char *)arr_at(&arr, 0));

//     c = 'c';
//     EXPECT_EQ(0, arr_set(&arr, 2, &c));
//     EXPECT_EQ('c', *(char *)arr_at(&arr, 2));
// }

// TEST_F(Table, arr_get) {
//     char c;

//     EXPECT_NE(0, arr_get(0, 0, 0));
//     EXPECT_NE(0, arr_get(&arr, 0, 0));
//     EXPECT_NE(0, arr_get(0, 0, &c));

//     c = 'a';
//     arr_insert(&arr, 0, &c);
//     c = 'b';
//     arr_insert(&arr, 0, &c);
//     c = 'c';
//     arr_insert(&arr, 0, &c);

//     EXPECT_EQ(0, arr_get(&arr, 0, &c));
//     EXPECT_EQ('c', c);

//     EXPECT_EQ(0, arr_get(&arr, 1, &c));
//     EXPECT_EQ('b', c);

//     EXPECT_EQ(0, arr_get(&arr, 2, &c));
//     EXPECT_EQ('a', c);

//     EXPECT_NE(0, arr_get(&arr, 3, &c));
//     // Not modified
//     EXPECT_EQ('a', c);
// }

// TEST_F(Table, arr_insert) {
//     char c;

//     // Invalid parameters
//     EXPECT_NE(0, arr_insert(0, 0, 0));
//     EXPECT_NE(0, arr_insert(&arr, 0, 0));
//     EXPECT_NE(0, arr_insert(0, 0, &c));

//     EXPECT_EQ(0, arr_size(&arr));

//     c = '1';
//     EXPECT_EQ(0, arr_insert(&arr, 0, &c));
//     EXPECT_EQ(1, arr_size(&arr));

//     c = '2';
//     EXPECT_EQ(0, arr_insert(&arr, 0, &c));
//     EXPECT_EQ(2, arr_size(&arr));

//     c = '3';
//     EXPECT_EQ(0, arr_insert(&arr, 2, &c));
//     EXPECT_EQ(3, arr_size(&arr));

//     // Grow
//     c = '4';
//     EXPECT_EQ(0, arr_insert(&arr, 3, &c));
//     EXPECT_EQ(4, arr_size(&arr));

//     // Past end
//     c = '5';
//     EXPECT_NE(0, arr_insert(&arr, 6, &c));
//     EXPECT_EQ(4, arr_size(&arr));

//     EXPECT_EQ('2', *(char *)arr_at(&arr, 0));
//     EXPECT_EQ('1', *(char *)arr_at(&arr, 1));
//     EXPECT_EQ('3', *(char *)arr_at(&arr, 2));
//     EXPECT_EQ('4', *(char *)arr_at(&arr, 3));
// }

// TEST_F(Table, array_insert_grow_fails) {
//     fill_array();

//     prealloc_fake.custom_fake = 0;
//     prealloc_fake.return_val  = 0;

//     char c = '1';
//     EXPECT_NE(0, arr_insert(&arr, 0, &c));
// }

// TEST_F(Table, arr_remove_start_only) {
//     char c = 'a';
//     arr_insert(&arr, 0, &c);
//     c = 'b';
//     arr_insert(&arr, 1, &c);
//     c = 'c';
//     arr_insert(&arr, 2, &c);
//     EXPECT_EQ(3, arr_size(&arr));
//     EXPECT_EQ('a', *(char *)arr_at(&arr, 0));
//     EXPECT_EQ('b', *(char *)arr_at(&arr, 1));
//     EXPECT_EQ('c', *(char *)arr_at(&arr, 2));

//     EXPECT_NE(0, arr_remove(0, 0, 0));
//     EXPECT_NE(0, arr_remove(0, 0, &c));
//     EXPECT_EQ(3, arr_size(&arr));

//     EXPECT_EQ(0, arr_remove(&arr, 0, &c));
//     EXPECT_EQ('a', c);
//     EXPECT_EQ(2, arr_size(&arr));

//     EXPECT_EQ(0, arr_remove(&arr, 0, &c));
//     EXPECT_EQ('b', c);
//     EXPECT_EQ(1, arr_size(&arr));

//     EXPECT_EQ(0, arr_remove(&arr, 0, &c));
//     EXPECT_EQ('c', c);
//     EXPECT_EQ(0, arr_size(&arr));

//     EXPECT_NE(0, arr_remove(&arr, 0, &c));
//     EXPECT_NE(0, arr_remove(&arr, 1, &c));

//     arr_insert(&arr, 0, &c);
//     EXPECT_EQ(0, arr_remove(&arr, 0, 0));
//     EXPECT_EQ(0, arr_size(&arr));
// }

// TEST_F(Table, arr_remove_end_only) {
//     char c = 'a';
//     arr_insert(&arr, 0, &c);
//     c = 'b';
//     arr_insert(&arr, 1, &c);
//     c = 'c';
//     arr_insert(&arr, 2, &c);
//     EXPECT_EQ(3, arr_size(&arr));
//     EXPECT_EQ('a', *(char *)arr_at(&arr, 0));
//     EXPECT_EQ('b', *(char *)arr_at(&arr, 1));
//     EXPECT_EQ('c', *(char *)arr_at(&arr, 2));

//     EXPECT_NE(0, arr_remove(0, 0, 0));
//     EXPECT_NE(0, arr_remove(0, 0, &c));
//     EXPECT_EQ(3, arr_size(&arr));

//     EXPECT_EQ(0, arr_remove(&arr, 2, &c));
//     EXPECT_EQ('c', c);
//     EXPECT_EQ(2, arr_size(&arr));

//     EXPECT_EQ(0, arr_remove(&arr, 1, &c));
//     EXPECT_EQ('b', c);
//     EXPECT_EQ(1, arr_size(&arr));

//     EXPECT_EQ(0, arr_remove(&arr, 0, &c));
//     EXPECT_EQ('a', c);
//     EXPECT_EQ(0, arr_size(&arr));

//     EXPECT_NE(0, arr_remove(&arr, 0, &c));
//     EXPECT_NE(0, arr_remove(&arr, 1, &c));
// }
