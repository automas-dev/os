#include "parser.h"

#include <stdbool.h>

#include "libc/memory.h"
#include "libc/stdio.h"
#include "libc/string.h"

extern int term_last_ret;

#define ERROR(MSG)                                \
    {                                             \
        printf(__FILE__ ":%u %s", __LINE__, MSG); \
    }

#define FATAL(MSG)         \
    {                      \
        ERROR(MSG)         \
        term_last_ret = 1; \
    }

static int take_quote(const char * str);
static int count_args(const char * line);

char ** parse_args(const char * line, size_t * out_len) {
    if (!line || !out_len) {
        return 0;
    }

    int len = count_args(line);
    if (len < 1) {
        return 0;
    }

    *out_len      = len;
    char ** args  = pmalloc(sizeof(char *) * len);
    size_t  arg_i = 0;

    while (*line) {
        if (arg_i > len) {
            FATAL("SYNTAX ERROR!\n");
            return 0;
        }
        // Skip whitespace
        while (*line && is_ws(*line)) {
            line++;
        }

        // end of string
        if (*line == 0) {
            break;
        }

        // Handle quote
        if (*line == '"') {
            int next = take_quote(line);
            if (next < 1) {
                return 0;
            }

            line++;

            args[arg_i] = pmalloc(sizeof(char) * next);
            kmemcpy(args[arg_i], line, next - 1);
            args[arg_i][next - 1] = 0;
            arg_i++;

            line += next;
            continue;
        }

        const char * start = line;
        // handle word
        while (*line && !is_ws(*line)) {
            line++;
        }

        size_t word_len = line - start;
        void * ptr      = pmalloc(sizeof(char) * word_len + 1);
        args[arg_i]     = ptr;
        kmemcpy(args[arg_i], start, word_len);
        args[arg_i][word_len] = 0;
        arg_i++;
    }

    if (arg_i < len) {
        FATAL("SYNTAX ERROR!\n");
        return 0;
    }

    return args;
}

bool is_ws(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\b';
}

static int take_quote(const char * str) {
    if (!str || *str != '"') {
        return -1;
    }

    size_t len = kstrlen(str);
    for (size_t i = 1; i < len; i++) {
        if (str[i] == '"' && str[i - 1] != '\\') {
            return i;
        }
    }

    return -1;
}

static int count_args(const char * line) {
    if (!line) {
        return -1;
    }

    int n = 0;
    while (*line) {
        // Skip whitespace
        while (*line && is_ws(*line)) {
            line++;
        }

        // end of string
        if (*line == 0) {
            break;
        }

        // Handle quote
        if (*line == '"') {
            int next = take_quote(line);
            if (next < 1) {
                return -1;
            }
            n++;
            line += 2 + next;
            continue;
        }

        // handle word
        while (*line && !is_ws(*line)) {
            line++;
        }
        n++;
    }
    return n;
}
