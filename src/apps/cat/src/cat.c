#include <stddef.h>

#include "libc/stdio.h"

int main(size_t argc, char ** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    file_t * file = file_open(argv[1], "r");

    if (!file) {
        printf("File not found %s\n", argv[1]);
        return 1;
    }

    char c;
    while (file_read(file, 1, 1, &c)) {
        putc(c);
    }

    file_close(file);

    return 0;
}
