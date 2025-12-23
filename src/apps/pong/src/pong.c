#include "libc/proc.h"
#include "libc/stdio.h"

void main() {
    for (;;) {
        puts("Pong\n");
        yield();
    }
}

void __start() {
    main();
}
