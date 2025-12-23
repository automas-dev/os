unit_test(
    TARGET test_vga
    TEST_FILES test_vga.cpp
    TARGET_FILES kernel/src/drivers/vga.c
)
