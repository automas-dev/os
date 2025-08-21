#ifndef IO_FILE_H
#define IO_FILE_H

#include <stddef.h>

enum IO_SEEK {
    IO_SEEK_BEGIN = 0,
    IO_SEEK_END,
    IO_SEEK_CURSOR,
};

typedef struct _io_file IO;

IO * open(const char * path, const char * mode);

int close(IO * io);

int read(IO * io, char *, size_t);

int write(IO * io, const char *, size_t);

int seek(IO * io, int, int);

int tell(IO * io);

#endif // IO_FILE_H
