#include <stddef.h>
#include <stdint.h>

#include "libc/stdio.h"
#include "libc/string.h"

int main(size_t argc, char ** argv) {
    bool next_line = true;
    if (argc > 1 && kmemcmp(argv[1], "-n", 2) == 0) {
        next_line = false;
    }

    size_t i = 1;
    if (!next_line) {
        i++;
    }
    for (; i < argc; i++) {
        puts(argv[i]);
        if (i < argc) {
            putc(' ');
        }
    }

    if (next_line) {
        putc('\n');
    }

    return 0;
}
