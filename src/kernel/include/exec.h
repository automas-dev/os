#ifndef EXEC_H
#define EXEC_H

#include "defs.h"

int command_exec(uint8_t * buff, const char * filepath, size_t size, size_t argc, char ** argv);

#endif // EXEC_H
