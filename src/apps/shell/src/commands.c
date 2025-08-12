#include "commands.h"

#include "libc/dir.h"
#include "libc/file.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "shell.h"

static int echo_cmd(size_t argc, char ** argv) {
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

static int ls_cmd(size_t argc, char ** argv) {
    dir_t dir = dir_open("/");
    if (!dir) {
        puts("Failed to open dir\n");
        return 1;
    }

    dir_seek(dir, 0, DIR_SEEK_ORIGIN_END);
    int n_files = dir_tell(dir);

    if (!n_files) {
        puts("Empty directory\n");
        dir_close(dir);
        return 0;
    }

    for (int i = 0; i < n_files; i++) {
        dir_entry_t d_entry;
        if (!dir_read(dir, &d_entry)) {
            printf("Failed to read file %d\n", i);
            dir_close(dir);
            return 1;
        }
        puts(d_entry.name);
        putc('\n');
    }

    dir_close(dir);

    return 0;
}

static int cat_cmd(size_t argc, char ** argv) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    file_t file = file_open(argv[1], "r");

    char c;
    while (file_read(file, 1, 1, &c)) {
        putc(c);
    }

    file_close(file);

    return 0;
}

void init_commands() {
    term_command_add("echo", echo_cmd);
    term_command_add("ls", ls_cmd);
    term_command_add("cat", cat_cmd);
}
