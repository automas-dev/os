#include "libc/proc.h"
#include "libc/stdio.h"

void init() {
    int pid = getpid();
    printf("Hello World from Init, pid %d (u%u 0x%X)\n", pid, pid, pid);
    yield();
    pid = getpid();
    printf("Welcome back! My PID is %d (u%u 0x%X)\n", pid, pid, pid);
    for (;;) {
        asm("hlt");
    }
}

void __start() {
    init();
}
