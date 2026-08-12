#ifndef KERNEL_IO_FS_H
#define KERNEL_IO_FS_H

#include "kernel/io.h"

enum IO_FS_FLAG {
    IO_FS_FLAG_READ  = 0x1,
    IO_FS_FLAG_WRITE = 0x2,
};

enum IO_FS_SEEK_ORIGIN {
    IO_FS_SEEK_ORIGIN_START,
    IO_FS_SEEK_ORIGIN_CURRENT,
    IO_FS_SEEK_ORIGIN_END,
};

typedef struct _io_fs_stat {
    int user;
    int group;
    int mode;

    int ctime;
    int atime;
    int mtime;

    size_t size;
} io_fs_stat_t;

struct _io_fs;
struct _io_fs_file;

typedef struct _io_fs_file * (*io_fs_file_open_t)(void * fs_data, const char * path, const char * mode);
typedef int (*io_fs_file_close_t)(void * fs_data, void * file_data);
typedef int (*io_fs_file_stat_t)(void * fs_data, const char * path, io_fs_stat_t * stat_out);

typedef size_t (*io_fs_file_read_t)(void * fs_data, void * file_data, char * buff, size_t size);
typedef size_t (*io_fs_file_write_t)(void * fs_data, void * file_data, const char * buff, size_t size);
typedef int (*io_fs_file_seek_t)(void * fs_data, void * file_data, int offset, int origin);
typedef int (*io_fs_file_tell_t)(void * fs_data, void * file_data);

typedef struct _io_fs {
    io_device_t * dev;
    int           flags;

    // TODO fs close

    io_fs_file_open_t file_open_fn;
    io_fs_file_stat_t file_stat_fn;

    // TODO directory iteration

    void * fs_data;
} io_fs_t;

typedef struct _io_fs_file {
    int          flags;
    const char * path;

    io_fs_file_close_t file_close_fn;

    io_fs_file_read_t  file_read_fn;
    io_fs_file_write_t file_write_fn;
    io_fs_file_seek_t  file_seek_fn;
    io_fs_file_tell_t  file_tell_fn;

    void * fs_data;
    void * file_data;
} io_fs_file_t;

io_fs_file_t * io_fs_file_open(io_fs_t * fs, const char * path, const char * mode);
int            io_fs_file_close(io_fs_file_t * file);
int            io_fs_file_stat(io_fs_t * fs, const char * path, io_fs_stat_t * stat_out);

size_t io_fs_file_read(io_fs_file_t * file, char * buff, size_t count);
size_t io_fs_file_write(io_fs_file_t * file, const char * buff, size_t count);
int    io_fs_file_seek(io_fs_file_t * file, int offset, int origin);
int    io_fs_file_tell(io_fs_file_t * file);

#endif // KERNEL_IO_FS_H
