#include "libc/proc.h"
#include "libc/stdio.h"

void main() {
    int pid = getpid();
    printf("PID is %u\n");
}
