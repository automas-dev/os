
#include "log.mock.h"

DEFINE_FAKE_VOID_FUNC_VARARG(kernel_log, int, const char *, size_t, const char *, const char *, ...);

void reset_log_mock() {
    RESET_FAKE(kernel_log);
}
