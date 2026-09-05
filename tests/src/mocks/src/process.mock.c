
#include "process.mock.h"

DEFINE_FAKE_VALUE_FUNC(int, process_create, process_t *);
DEFINE_FAKE_VALUE_FUNC(int, process_free, process_t *);
DEFINE_FAKE_VALUE_FUNC(int, process_set_entrypoint, process_t *, void *);
DEFINE_FAKE_VALUE_FUNC(int, process_resume, process_t *, const ebus_event_t *);
DEFINE_FAKE_VALUE_FUNC(void *, process_add_pages, process_t *, size_t);
DEFINE_FAKE_VALUE_FUNC(int, process_grow_stack, process_t *);
DEFINE_FAKE_VALUE_FUNC(int, process_load_heap, process_t *, const char *, size_t);
DEFINE_FAKE_VOID_FUNC(set_active_task, process_t *);
DEFINE_FAKE_VALUE_FUNC(process_t *, get_active_task);
DEFINE_FAKE_VOID_FUNC(switch_task, process_t *);
DEFINE_FAKE_VOID_FUNC(enter_usermode);

void reset_process_mock() {
    RESET_FAKE(process_create);
    RESET_FAKE(process_free);
    RESET_FAKE(process_set_entrypoint);
    RESET_FAKE(process_resume);
    RESET_FAKE(process_add_pages);
    RESET_FAKE(process_grow_stack);
    RESET_FAKE(process_load_heap);
    RESET_FAKE(set_active_task);
    RESET_FAKE(get_active_task);
    RESET_FAKE(switch_task);
    RESET_FAKE(enter_usermode);
}
