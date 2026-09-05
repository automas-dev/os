#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "fff.h"
#include "process.h"

DECLARE_FAKE_VALUE_FUNC(int, process_create, process_t *);
DECLARE_FAKE_VALUE_FUNC(int, process_free, process_t *);
DECLARE_FAKE_VALUE_FUNC(int, process_set_entrypoint, process_t *, void *);
DECLARE_FAKE_VALUE_FUNC(int, process_resume, process_t *, const ebus_event_t *);
DECLARE_FAKE_VALUE_FUNC(void *, process_add_pages, process_t *, size_t);
DECLARE_FAKE_VALUE_FUNC(int, process_grow_stack, process_t *);
DECLARE_FAKE_VALUE_FUNC(int, process_load_heap, process_t *, const char *, size_t);
DECLARE_FAKE_VOID_FUNC(set_active_task, process_t *);
DECLARE_FAKE_VALUE_FUNC(process_t *, get_active_task);
DECLARE_FAKE_VOID_FUNC(switch_task, process_t *);
DECLARE_FAKE_VOID_FUNC(enter_usermode);

void reset_process_mock(void);

#ifdef __cplusplus
} // extern "C"
#endif
