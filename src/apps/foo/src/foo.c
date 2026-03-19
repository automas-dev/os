#include "libc/proc.h"
#include "libc/stdio.h"

void foo() {
    uint32_t res = puts("Hello\n");

    pull_event(EBUS_EVENT_KEY, 0);

    res = printf("Hello 0x%X\n", res);

    printf("There are %u res and it has %c with %d left\n", res, 'c', 1800);

    proc_exit(0);
}

void main() {
    foo();
}
