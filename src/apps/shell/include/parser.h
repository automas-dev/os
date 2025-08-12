#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stddef.h>

bool    is_ws(char c);
char ** parse_args(const char * line, size_t * out_len);

#endif // PARSER_H
