unit_test(
    TARGET test_mmu
    TEST_FILES test_mmu.cpp
    TARGET_FILES cpu/i386-elf/src/mmu.c
)

unit_test(
    TARGET test_gdt
    TEST_FILES test_gdt.cpp
    TARGET_FILES cpu/i386-elf/src/gdt.c
)
