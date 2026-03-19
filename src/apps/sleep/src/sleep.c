#include <stddef.h>

#include "libc/stdio.h"
#include "libc/string.h"
#include "libc/time.h"

int main(size_t argc, char ** argv) {
    if (argc < 2 || (!kstrcmp(argv[1], "-u") && argc < 3)) {
        printf("Usage: %s [-u] <seconds>\n", argv[0]);
        puts("\nseconds must be at while number that is least 1\n");
        puts("-u    use microseconds instead of seconds\n");
        return -1;
    }

    if (!kstrcmp(argv[1], "-u")) {
        int us = katoi(argv[2]);
        if (us < 1) {
            printf("Seconds must be a numer that is at least 1\n");
            return -1;
        }

        printf("Sleeping for %d microseconds\n", us);
        usleep(us);
    }
    else {
        int seconds = katoi(argv[1]);
        if (seconds < 1) {
            printf("Seconds must be a numer that is at least 1\n");
            return -1;
        }

        printf("Sleeping for %d sseconds\n", seconds);
        sleep(seconds * 1000);
    }

    puts("Finished sleep\n");

    return 0;
}
