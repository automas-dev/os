#include "commands.h"

#include "libc/dir.h"
#include "libc/file.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "libc/time.h"
#include "shell.h"

static int ls_cmd(size_t argc, char ** argv) {
    // dir_t dir = dir_open("/");
    // if (!dir) {
    //     puts("Failed to open dir\n");
    //     return 1;
    // }

    // dir_seek(dir, 0, DIR_SEEK_ORIGIN_END);
    // int n_files = dir_tell(dir);

    // if (!n_files) {
    //     puts("Empty directory\n");
    //     dir_close(dir);
    //     return 0;
    // }

    // for (int i = 0; i < n_files; i++) {
    //     dir_entry_t d_entry;
    //     if (!dir_read(dir, &d_entry)) {
    //         printf("Failed to read file %d\n", i);
    //         dir_close(dir);
    //         return 1;
    //     }
    //     puts(d_entry.name);
    //     putc('\n');
    // }

    // dir_close(dir);

    return 0;
}

static int pid_cmd(size_t argc, char ** argv) {
    printf("PID is %d\n", getpid());
    return 0;
}

void init_commands() {
    term_command_add("ls", ls_cmd);
    term_command_add("pid", pid_cmd);
}
