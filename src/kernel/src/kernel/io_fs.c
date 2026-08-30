#define KLOG_SERVICE "KERNEL/IO_FS"

#include "kernel/io/fs.h"
#include "kernel/logs.h"
#include "kernel/memory.h"

int io_fs_close(io_fs_t * fs) {
    if (!fs) {
        KLOG_WARNING("io_fs_file_open received a null pointer for fs struct");
        return 0;
    }

    int close_res = fs->close_fn(fs->fs_data);
    if (!close_res) {
        KLOG_DEBUG("Filesystem close function returned an error %d", close_res);
    }

    kfree(fs);
    return close_res;
}

io_device_t * io_fs_file_open(io_fs_t * fs, const char * path, const char * mode) {
    if (!fs) {
        KLOG_WARNING("io_fs_file_open received a null pointer for fs struct");
        return 0;
    }

    if (!fs->file_open_fn) {
        KLOG_WARNING("io_fs_file_open called on filesystem with missing required file open function");
        return 0;
    }

    return fs->file_open_fn(fs->fs_data, path, mode);
}

int io_fs_file_stat(io_fs_t * fs, const char * path, io_fs_stat_t * stat_out) {
    if (!fs) {
        KLOG_WARNING("io_fs_file_stat received a null pointer for fs struct");
        return -1;
    }

    if (!fs->file_stat_fn) {
        KLOG_WARNING("io_fs_file_stat called on filesystem with missing required file stat function");
        return 0;
    }

    return fs->file_stat_fn(fs->fs_data, path, stat_out);
}
