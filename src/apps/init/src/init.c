#include "libc/proc.h"
#include "libc/stdio.h"

void init() {
    printf("Hello World from Init\n");
    for (;;) {
        asm("hlt");
    }
}

void __start() {
    init();
}
