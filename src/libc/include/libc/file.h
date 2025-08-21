#ifndef LIBC_FILE_H
#define LIBC_FILE_H

#include <stddef.h>

typedef int (*_libc_config_file_write_call_fn)(int handle, const char * buff, size_t count, size_t pos);

enum FILE_FLAG {
    FILE_FLAG_READ  = 0x1,
    FILE_FLAG_WRITE = 0x2,
    FILE_FLAG_SIZED = 0x4,
};

#define FILE_FLAG_RW (FILE_FLAG_READ | FILE_FLAG_WRITE)

typedef struct _libc_file {
    int handle;

    int flags;

    size_t size;
    size_t pos;
} file_t;

enum FILE_SEEK_ORIGIN {
    FILE_SEEK_ORIGIN_CURSOR,
    FILE_SEEK_ORIGIN_START,
    FILE_SEEK_ORIGIN_END,
};

file_t * file_open(const char * path, const char * mode);
int      file_close(file_t * file);
size_t   file_read(file_t * file, size_t size, size_t count, void * buff);
size_t   file_write(file_t * file, size_t size, size_t count, const void * buff);
int      file_seek(file_t * file, int offset, int origin);
int      file_tell(file_t * file);

void _libc_config_file_write_call(_libc_config_file_write_call_fn);

#endif // LIBC_FILE_H
