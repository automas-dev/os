#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "kernel/memory.h"

DECLARE_FAKE_VOID_FUNC(kmalloc_init, size_t);
DECLARE_FAKE_VALUE_FUNC(void *, kmalloc, size_t);
DECLARE_FAKE_VALUE_FUNC(void *, krealloc, void *, size_t);
DECLARE_FAKE_VOID_FUNC(kfree, void *);
DECLARE_FAKE_VALUE_FUNC(void *, kernel_alloc_page, size_t);

void reset_kernel_memory_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
