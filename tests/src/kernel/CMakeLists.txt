unit_test(
    TARGET test_paging
    TEST_FILES test_paging.cpp
    TARGET_FILES kernel/src/paging.c
)

unit_test(
    TARGET test_ram
    TEST_FILES test_ram.cpp
    TARGET_FILES kernel/src/ram.c
)
