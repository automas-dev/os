#ifndef KERNEL_IO_FS_H
#define KERNEL_IO_FS_H

#include "kernel/io.h"

typedef struct _io_fs_stat {
    int user;
    int group;
    int mode;

    int ctime;
    int atime;
    int mtime;

    size_t size;
} io_fs_stat_t;

typedef int (*io_fs_close_t)(void * fs_data);

typedef io_device_t * (*io_fs_file_open_t)(void * fs_data, const char * path, const char * mode);
typedef int (*io_fs_file_stat_t)(void * fs_data, const char * path, io_fs_stat_t * stat_out);

typedef struct _io_fs {
    io_device_t * dev;

    // Required
    io_fs_close_t     close_fn;
    io_fs_file_open_t file_open_fn;
    io_fs_file_stat_t file_stat_fn;

    // TODO directory iteration

    void * fs_data;
} io_fs_t;

int           io_fs_close(io_fs_t * fs);
io_device_t * io_fs_file_open(io_fs_t * fs, const char * path, const char * mode);
int           io_fs_file_stat(io_fs_t * fs, const char * path, io_fs_stat_t * stat_out);

#endif // KERNEL_IO_FS_H
