#include "libc/proc.h"
#include "libc/stdio.h"

void __start() {
    int pid = getpid();
    printf("PID is %u\n");
}
