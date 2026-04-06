#include "libc/file.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"

void init() {
    // TODO why doesn't keyboard work without this?
    yield();

    int shell_pid = proc_open("shell", 0, 0);
    proc_set_foreground(shell_pid);

    proc_wait_pid(shell_pid, 0);
    printf("Shell exited\n");
}

void __start() {
    init();
}
