#include "libc/time.h"

#include "ebus.h"
#include "libk/sys_call.h"

size_t time() {
    return _sys_event_time();
}

void sleep(size_t ms) {
    _sys_event_sleep(ms, 0);
}

void usleep(size_t us) {
    _sys_event_sleep(0, us);
}
