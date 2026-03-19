#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "libk/sys_call.h"

DECLARE_FAKE_VALUE_FUNC(void *, _sys_page_alloc, size_t);
DECLARE_FAKE_VOID_FUNC(_sys_proc_exit, int);
DECLARE_FAKE_VOID_FUNC(_sys_proc_abort, int, const char *);
DECLARE_FAKE_VOID_FUNC(_sys_proc_panic, const char *, const char *, unsigned int);
DECLARE_FAKE_VALUE_FUNC(int, _sys_proc_getpid);
DECLARE_FAKE_VOID_FUNC(_sys_register_signals, void *);
DECLARE_FAKE_VOID_FUNC(_sys_queue_event, ebus_event_t *);
DECLARE_FAKE_VOID_FUNC(_sys_yield);
DECLARE_FAKE_VALUE_FUNC(size_t, _sys_putc, char);
DECLARE_FAKE_VALUE_FUNC(size_t, _sys_puts, const char *);

void reset_libk_sys_call_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
