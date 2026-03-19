#include <stddef.h>
#include <stdint.h>

#include "libc/stdio.h"

int main(size_t argc, char ** argv) {
    printf("Bar got %u arguments\n", argc);

    for (size_t i = 0; i < argc; i++) {
        printf("Argument %u is ", i);
        puts(argv[i]);
        putc('\n');
    }

    return 0;
}
