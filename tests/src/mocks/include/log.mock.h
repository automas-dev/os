#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// #include "kernel/log.h"
#include "fff.h"

DECLARE_FAKE_VOID_FUNC_VARARG(kernel_log, int, const char *, size_t, const char *, const char *, ...);

void reset_log_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
