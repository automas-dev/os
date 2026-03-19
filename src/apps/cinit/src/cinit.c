#include "libc/proc.h"
#include "libc/stdio.h"

extern int main(size_t argc, char ** argv);

void __cinit(size_t argc, char ** argv) {
    printf("c init\n");
    // TODO init malloc
    // TODO is there anything in signals or system calls to setup?
    // TODO do stdio handles setup here?
    int res = main(argc, argv);
    printf("Main returned %d\n", res);
    proc_exit(res);
}
