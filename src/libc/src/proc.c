#include "libc/proc.h"

#include "libk/sys_call.h"

static _libc_config_queue_event_fn __queue_event = _sys_queue_event;

void proc_exit(uint8_t code) {
    _sys_proc_exit(code);
}

void proc_abort(uint8_t code, const char * msg) {
    _sys_proc_abort(code, msg);
}

NO_RETURN void proc_panic(const char * msg, const char * file, unsigned int line) {
    _sys_proc_panic(msg, file, line);
}

void queue_event(ebus_event_t * event) {
    __queue_event(event);
}

int pull_event(int filter, ebus_event_t * event_out) {
    return _sys_yield(filter, event_out);
}

void yield() {
    _sys_yield(0, 0);
}

int proc_open(const char * filename, size_t argc, char ** argv) {
    if (!filename) {
        return -1;
    }
    return _sys_proc_exec(filename, argc, argv);
}

int getpid(void) {
    return _sys_proc_getpid();
}

int proc_set_foreground(int pid) {
    _sys_proc_set_foreground(pid);
}

void _libc_config_queue_event_call(_libc_config_queue_event_fn fn) {
    __queue_event = fn;
}
