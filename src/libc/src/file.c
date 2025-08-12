#include "libc/file.h"

#include "libk/sys_call.h"

file_t file_open(const char * path, const char * mode) {
    return _sys_io_file_open(path, mode);
}

void file_close(file_t fp) {
    _sys_io_file_close(fp);
}

size_t file_read(file_t fp, size_t size, size_t count, void * buff) {
    return _sys_io_file_read(fp, size, count, buff);
}

size_t file_write(file_t fp, size_t size, size_t count, const void * buff) {
    return _sys_io_file_write(fp, size, count, buff);
}

int file_seek(file_t fp, int offset, int origin) {
    return _sys_io_file_seek(fp, offset, origin);
}

int file_tell(file_t fp) {
    return _sys_io_file_tell(fp);
}
