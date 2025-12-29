#include "libc/proc.h"
#include "libc/stdio.h"

void main() {
    for (;;) {
        puts("Ping\n");
        yield();
    }
}

void __start() {
    main();
}
