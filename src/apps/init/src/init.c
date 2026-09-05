#include "libc/file.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libk/sys_call.h"

int main(size_t argc, char ** argv) {
    const char * description = _sys_kernel_describe();
    printf("%s\n", description);

    // TODO why doesn't keyboard work without this?
    yield();

    int shell_pid = proc_open("shell", 0, 0);
    proc_set_foreground(shell_pid);

    proc_wait_pid(shell_pid, 0);
    printf("Shell exited\n");

    return 0;
}
