unit_test(
    TARGET test_memory_alloc
    TEST_FILES test_memory_alloc.cpp
    TARGET_FILES kernel/src/memory_alloc.c
)

unit_test(
    TARGET test_paging
    TEST_FILES test_paging.cpp
    TARGET_FILES kernel/src/paging.c
)

unit_test(
    TARGET test_ram
    TEST_FILES test_ram.cpp
    TARGET_FILES kernel/src/drivers/ram.c
)
