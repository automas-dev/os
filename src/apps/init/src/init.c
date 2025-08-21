#include "libc/file.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"

void init() {
    int pid = getpid();
    printf("Hello World from Init, pid %d (u%u 0x%X)\n", pid, pid, pid);
    yield();
    pid = getpid();
    printf("Welcome back! My PID is %d (u%u 0x%X)\n", pid, pid, pid);

    file_t * f = file_open("data/a.txt", "r");
    printf("File pointer is %p\n", f);

    if (f) {
        file_seek(f, 0, FILE_SEEK_ORIGIN_END);
        size_t file_size = file_tell(f);
        printf("File a.txt has size %u\n", file_size);
        file_seek(f, 0, FILE_SEEK_ORIGIN_START);

        char * buff  = pmalloc(file_size);
        size_t o_len = file_read(f, 1, file_size, buff);

        printf("Read %u bytes\n", o_len);

        printf("Message is ");
        puts(buff);

        file_close(f);
    }

    for (;;) {
        asm("hlt");
    }
}

void __start() {
    init();
}
