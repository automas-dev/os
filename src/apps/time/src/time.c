#include "libc/time.h"

#include <stddef.h>

#include "libc/stdio.h"

int main(size_t argc, char ** argv) {
    printf("The time is %u seconds\n", time());

    return 0;
}
