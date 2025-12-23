#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "libc/proc.h"

DECLARE_FAKE_VOID_FUNC(proc_exit, uint8_t);
DECLARE_FAKE_VOID_FUNC(proc_abort, uint8_t, const char *);
DECLARE_FAKE_VOID_FUNC(proc_panic, const char *, const char *, unsigned int);
DECLARE_FAKE_VOID_FUNC(set_next_pid, uint32_t);
DECLARE_FAKE_VOID_FUNC(queue_event, ebus_event_t *);
DECLARE_FAKE_VALUE_FUNC(int, pull_event, int, ebus_event_t *);
DECLARE_FAKE_VALUE_FUNC(int, proc_open, const char *, size_t, char **);
DECLARE_FAKE_VOID_FUNC(yield);

void reset_libc_proc_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
