#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>
#include <stddef.h>

typedef int (*command_cb_t)(size_t argc, char ** argv);

bool term_command_add(const char * command, command_cb_t cb);

#endif // SHELL_H
