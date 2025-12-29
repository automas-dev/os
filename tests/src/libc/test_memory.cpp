// #include <cstdlib>
// #include <cstring>
// #include <string>

// #include "test_common.h"

// extern "C" {
// #include "libc/memory.h"
// #include "memory_alloc.h"

// static memory_t memory;
// }

// class LibC : public ::testing::Test {
// protected:
//     void SetUp() override {
//         init_mocks();

//         memset(&memory, 0, sizeof(memory_t));

//         // init_malloc(&memory);
//     }
// };

// TEST_F(LibC, pmalloc) {
//     void * ptr = pmalloc(12);
//     EXPECT_EQ(1, memory_alloc_fake.call_count);
//     EXPECT_EQ(&memory, memory_alloc_fake.arg0_val);
//     EXPECT_EQ(12, memory_alloc_fake.arg1_val);
// }

// TEST_F(LibC, prealloc) {
//     char buff[12];

//     void * ptr = prealloc(buff, 12);
//     EXPECT_EQ(1, memory_realloc_fake.call_count);
//     EXPECT_EQ(&memory, memory_realloc_fake.arg0_val);
//     EXPECT_EQ(buff, memory_realloc_fake.arg1_val);
//     EXPECT_EQ(12, memory_realloc_fake.arg2_val);
// }

// TEST_F(LibC, pfree) {
//     char buff[12];
//     pfree(buff);

//     EXPECT_EQ(1, memory_free_fake.call_count);
//     EXPECT_EQ(&memory, memory_free_fake.arg0_val);
//     EXPECT_EQ(buff, memory_free_fake.arg1_val);
// }
