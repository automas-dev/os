#ifndef LIBC_FILE_H
#define LIBC_FILE_H

#include <stddef.h>

typedef int file_t;

enum FILE_SEEK_ORIGIN {
    FILE_SEEK_ORIGIN_CURSOR,
    FILE_SEEK_ORIGIN_START,
    FILE_SEEK_ORIGIN_END,
};

file_t file_open(const char * path, const char * mode);
void   file_close(file_t fp);
size_t file_read(file_t fp, size_t size, size_t count, void * buff);
size_t file_write(file_t fp, size_t size, size_t count, const void * buff);
int    file_seek(file_t fp, int offset, int origin);
int    file_tell(file_t fp);

#endif // LIBC_FILE_H
