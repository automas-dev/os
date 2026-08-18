#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "kernel/io_buffer.h"

DECLARE_FAKE_VALUE_FUNC(io_buffer_t *, io_buffer_create, size_t);
DECLARE_FAKE_VOID_FUNC(io_buffer_free, io_buffer_t *);

void reset_kernel_io_buffer_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
