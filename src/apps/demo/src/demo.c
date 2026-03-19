#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"

int main(size_t argc, char ** argv) {
    printf("Lets demo some cool features of printf\n");
    int len = printf("Like the percent sign %%, \na signed int %d, a signed int with width formatting %4d, \nleading zeros %04d, left align %-4d\n", 10, 10, 10, 10);
    len += printf("How about negative numbers: signed %d and unsigned %u\n", -10, -10);
    len += printf("Now for non decimal 0x%04x and 0x%04X or octal %o\n", 1234, 1234, 1234);
    len += printf("There's booleans to %b and chars like %c and strings like %s\n", true, 'c', "this");
    int store = 0;
    len += printf("The last part is pointers %8p\n", &store);

    void * data = pmalloc(10);

    printf("\nMalloc memory got pointer %p\n", data);
    printf("Float number %f or shorter %3f or digits %.4f or lead %.04f\n", 3.14, 31.45, 3.14, 3.14);
    printf("%f\n", 12345678.0);

    proc_exit(0);

    return 0;
}
