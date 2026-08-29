#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "kernel/device/screen.h"

DECLARE_FAKE_VALUE_FUNC(io_device_t *, io_device_screen_open);
DECLARE_FAKE_VALUE_FUNC(int, io_device_screen_write_raw, int, const char *, size_t, size_t);

void reset_kernel_screen_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
