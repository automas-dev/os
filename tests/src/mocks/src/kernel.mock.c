#include "kernel/memory.mock.h"
#include "kernel/io_buffer.mock.h"
#include "kernel/device/screen.mock.h"
#include "kernel/panic.mock.h"

DEFINE_FAKE_VOID_FUNC(kmalloc_init, size_t);
DEFINE_FAKE_VALUE_FUNC(void *, kmalloc, size_t);
DEFINE_FAKE_VALUE_FUNC(void *, krealloc, void *, size_t);
DEFINE_FAKE_VOID_FUNC(kfree, void *);
DEFINE_FAKE_VALUE_FUNC(void *, kernel_alloc_page, size_t);

void reset_kernel_memory_mock(void) {
    RESET_FAKE(kmalloc_init);
    RESET_FAKE(kmalloc);
    RESET_FAKE(krealloc);
    RESET_FAKE(kfree);
    RESET_FAKE(kernel_alloc_page);
}

DEFINE_FAKE_VALUE_FUNC(io_buffer_t *, io_buffer_create, size_t);
DEFINE_FAKE_VOID_FUNC(io_buffer_free, io_buffer_t *);

void reset_kernel_io_buffer_mock(void) {
    RESET_FAKE(io_buffer_create);
    RESET_FAKE(io_buffer_free);
}

DEFINE_FAKE_VALUE_FUNC(io_device_t *, io_device_screen_open);
DEFINE_FAKE_VALUE_FUNC(int, io_device_screen_write_raw, int, const char *, size_t, size_t);

void reset_kernel_screen_mock(void) {
    RESET_FAKE(io_device_screen_open);
    RESET_FAKE(io_device_screen_write_raw);
}

DEFINE_FAKE_VOID_FUNC(kernel_panic, const char *, const char *, unsigned int);

void reset_kernel_panic_mock(void) {
    RESET_FAKE(kernel_panic);
}
