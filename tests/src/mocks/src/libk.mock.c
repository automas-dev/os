#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "libk/sys_call.mock.h"

// libk/sys_call.h

DEFINE_FAKE_VALUE_FUNC(void *, _sys_page_alloc, size_t);
DEFINE_FAKE_VOID_FUNC(_sys_proc_exit, int);
DEFINE_FAKE_VOID_FUNC(_sys_proc_abort, int, const char *);
DEFINE_FAKE_VOID_FUNC(_sys_proc_panic, const char *, const char *, unsigned int);
DEFINE_FAKE_VALUE_FUNC(int, _sys_proc_getpid);
DEFINE_FAKE_VOID_FUNC(_sys_register_signals, void *);
DEFINE_FAKE_VOID_FUNC(_sys_queue_event, ebus_event_t *);
DEFINE_FAKE_VOID_FUNC(_sys_yield);
DEFINE_FAKE_VALUE_FUNC(size_t, _sys_putc, char);
DEFINE_FAKE_VALUE_FUNC(size_t, _sys_puts, const char *);

void reset_libk_sys_call_mock(void) {
    RESET_FAKE(_sys_page_alloc);
    RESET_FAKE(_sys_proc_exit);
    RESET_FAKE(_sys_proc_abort);
    RESET_FAKE(_sys_proc_panic);
    RESET_FAKE(_sys_proc_getpid);
    RESET_FAKE(_sys_register_signals);
    RESET_FAKE(_sys_queue_event);
    RESET_FAKE(_sys_yield);
    RESET_FAKE(_sys_putc);
    RESET_FAKE(_sys_puts);
}
