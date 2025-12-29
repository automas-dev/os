#include "libc/file.h"

#include "libc/memory.h"
#include "libc/string.h"
#include "libk/sys_call.h"

static _libc_config_file_write_call_fn __file_write = _sys_io_write;

file_t * file_open(const char * path, const char * mode) {
    if (!path || !mode) {
        return 0;
    }

    int handle = _sys_io_open(path, mode);
    if (handle <= 0) {
        return 0;
    }

    file_t * file = pmalloc(sizeof(file_t));
    if (!file) {
        _sys_io_close(handle);
        return 0;
    }

    kmemset(file, 0, sizeof(file_t));

    file->handle = handle;

    for (; *mode; mode++) {
        switch (*mode) {
            case 'r':
                file->flags |= FILE_FLAG_READ;
                break;
            case 'w':
                file->flags |= FILE_FLAG_WRITE;
                break;
        }
    }

    int size = _sys_io_size(handle);
    if (size >= 0) {
        file->flags |= FILE_FLAG_SIZED;
        file->size = size;
    }

    return file;
}

int file_close(file_t * file) {
    if (!file) {
        return -1;
    }

    if (_sys_io_close(file->handle)) {
        return -1;
    }

    pfree(file);

    return 0;
}

size_t file_read(file_t * file, size_t size, size_t count, void * buff) {
    if (!file || !(file->flags & FILE_FLAG_READ)) {
        return 0;
    }

    size_t len = _sys_io_read(file->handle, buff, size * count, file->pos);

    if (file->flags & FILE_FLAG_SIZED) {
        file->pos += len;
    }

    return len;
}

size_t file_write(file_t * file, size_t size, size_t count, const void * buff) {
    if (!file || !(file->flags & FILE_FLAG_WRITE)) {
        return 0;
    }

    size_t len = __file_write(file->handle, buff, size * count, file->pos);

    if (file->flags & FILE_FLAG_SIZED) {
        file->pos += len;
    }

    return len;
}

int file_seek(file_t * file, int offset, int origin) {
    if (!file) {
        return -1;
    }

    if (!(file->flags & FILE_FLAG_SIZED)) {
        return 0;
    }

    switch (origin) {
        case FILE_SEEK_ORIGIN_CURSOR: {
            if (offset < 0) {
                if (-offset > file->pos) {
                    offset = -file->pos;
                }
            }
            else {
                if (file->pos + offset > file->size) {
                    offset = file->size - file->pos;
                }
            }
            file->pos += offset;
        } break;

        case FILE_SEEK_ORIGIN_START: {
            if (offset < 0) {
                return -1;
            }
            if (offset > file->size) {
                offset = file->size;
            }
            file->pos = offset;
        } break;

        case FILE_SEEK_ORIGIN_END: {
            if (offset > 0) {
                return -1;
            }
            if (-offset > file->size) {
                offset = -file->size;
            }
            file->pos = file->size - offset;
        } break;

        default:
            return -1;
    }

    return 0;
}

int file_tell(file_t * file) {
    if (!file) {
        return -1;
    }

    if (!(file->flags & FILE_FLAG_SIZED)) {
        return 0;
    }

    return file->pos;
}

void _libc_config_file_write_call(_libc_config_file_write_call_fn fn) {
    __file_write = fn;
}
