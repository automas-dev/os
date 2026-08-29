#define KLOG_SERVICE "KERNEL/IO_FS"

#include "kernel/io/fs.h"
#include "kernel/logs.h"

io_fs_file_t * io_fs_file_open(io_fs_t * fs, const char * path, const char * mode) {
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

int io_fs_file_close(io_fs_file_t * file) {
    if (!file) {
        KLOG_WARNING("io_fs_file_close received a null pointer for file struct");
        return -1;
    }

    if (!file->file_close_fn) {
        KLOG_WARNING("io_fs_file_close called on file with missing required close function");
        return 0;
    }

    return file->file_close_fn(file->fs_data, file->file_data);
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

size_t io_fs_file_read(io_fs_file_t * file, char * buff, size_t count) {
    if (!file) {
        KLOG_WARNING("io_fs_file_read received a null pointer for file struct");
        return 0;
    }

    if (!(file->flags & IO_FS_FILE_FLAG_READ)) {
        KLOG_WARNING("io_fs_file_read called on file without read flag");
        return 0;
    }

    if (!file->file_read_fn) {
        KLOG_WARNING("io_fs_file_read called on file with no read function");
        return 0;
    }

    return file->file_read_fn(file->fs_data, file->file_data, buff, count);
}

size_t io_fs_file_write(io_fs_file_t * file, const char * buff, size_t count) {
    if (!file) {
        KLOG_WARNING("io_fs_file_write received a null pointer for file struct");
        return 0;
    }

    if (!(file->flags & IO_FS_FILE_FLAG_WRITE)) {
        KLOG_WARNING("io_fs_file_write called on file without write flag");
        return 0;
    }

    if (!file->file_write_fn) {
        KLOG_WARNING("io_fs_file_write called on file with no write function");
        return 0;
    }

    return file->file_write_fn(file->fs_data, file->file_data, buff, count);
}

int io_fs_file_seek(io_fs_file_t * file, int offset, int origin) {
    if (!file) {
        KLOG_WARNING("io_fs_file_seek received a null pointer for file");
        return -1;
    }

    if (!file->file_seek_fn) {
        KLOG_WARNING("io_fs_file_seek called on file with missing required seek function");
        return 0;
    }

    return file->file_seek_fn(file->fs_data, file->file_data, offset, origin);
}

int io_fs_file_tell(io_fs_file_t * file) {
    if (!file) {
        KLOG_WARNING("io_fs_file_tell received a null pointer for file");
        return -1;
    }

    if (!file->file_tell_fn) {
        KLOG_WARNING("io_fs_file_tell called on file with missing required tell function");
        return 0;
    }

    return file->file_tell_fn(file->fs_data, file->file_data);
}
