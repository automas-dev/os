#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "kernel/panic.h"

DECLARE_FAKE_VOID_FUNC(kernel_panic, const char *, const char *, unsigned int);

void reset_kernel_panic_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
