#include "libc/signal.h"

#include "libc/memory.h"
#include "libc/proc.h"
#include "libk/sys_call.h"

typedef struct _signal {
    int              sig_no;
    signal_handler   callback;
    struct _signal * next;
} signal_t;

static signal_t * __signals = 0;

static void signal_callback(int sig_no) {
    signal_t * sig = __signals;
    while (sig) {
        if (sig->sig_no == sig_no) {
            sig->callback();
            break;
        }
        sig = sig->next;
    }
}

int register_signal(int sig_no, signal_handler callback) {
    if (!callback) {
        return -1;
    }

    signal_t * sig = __signals;
    while (sig) {
        if (sig->sig_no == sig_no) {
            return -1;
        }
        sig = sig->next;
    }

    sig = pmalloc(sizeof(signal_t));
    if (!sig) {
        return -1;
    }

    sig->sig_no   = sig_no;
    sig->callback = callback;
    sig->next     = 0;

    sig->next = __signals;
    __signals = sig;

    _sys_register_signals(signal_callback);
    return 0;
}
