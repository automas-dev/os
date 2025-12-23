unit_test(
    TARGET test_libc_circular_buffer
    TEST_FILES test_circular_buffer.cpp
    TARGET_FILES libc/src/circular_buffer.c
)

unit_test(
    TARGET test_libc_array
    TEST_FILES test_array.cpp
    TARGET_FILES libc/src/array.c
)

# unit_test(
#     TARGET test_libc_memory
#     TEST_FILES test_memory.cpp
#     TARGET_FILES libc/src/memory.c
# )

# unit_test(
#     TARGET test_libc_process
#     TEST_FILES test_process.cpp
#     TARGET_FILES libc/src/proc.c
# )

unit_test(
    TARGET test_libc_signal
    TEST_FILES test_signal.cpp
    TARGET_FILES libc/src/signal.c
)

unit_test(
    TARGET test_libc_string
    TEST_FILES test_string.cpp
    TARGET_FILES libc/src/string.c
)
