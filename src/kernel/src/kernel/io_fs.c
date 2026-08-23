#define KLOG_SERVICE "KERNEL/IO_FS"

#include "kernel/io/fs.h"
#include "kernel/logs.h"

// TODO should null ptr checks be here or in each read function?

io_fs_file_t * io_fs_file_open(io_fs_t * fs, const char * path, const char * mode) {
    if (!fs) {
        KLOG_ERROR("io_fs_file_open received a null pointer for fs struct");
        return 0;
    }
    if (!path) {
        KLOG_ERROR("io_fs_file_open received a null pointer for path");
        return 0;
    }
    if (!mode) {
        KLOG_ERROR("io_fs_file_open received a null pointer for mode");
        return 0;
    }

    return fs->file_open_fn(fs->fs_data, path, mode);
}

int io_fs_file_close(io_fs_file_t * file) {
    if (!file) {
        KLOG_ERROR("io_fs_file_close received a null pointer for file struct");
        return -1;
    }

    return file->file_close_fn(file->fs_data, file->file_data);
}

int io_fs_file_stat(io_fs_t * fs, const char * path, io_fs_stat_t * stat_out) {
    if (!fs) {
        KLOG_ERROR("io_fs_file_stat received a null pointer for fs struct");
        return -1;
    }
    if (!path) {
        KLOG_ERROR("io_fs_file_stat received a null pointer for path");
        return -1;
    }
    if (!stat_out) {
        KLOG_ERROR("io_fs_file_stat received a null pointer for stat_out");
        return -1;
    }

    return fs->file_stat_fn(fs->fs_data, path, stat_out);
}

size_t io_fs_file_read(io_fs_file_t * file, char * buff, size_t count) {
    if (!file) {
        KLOG_ERROR("io_fs_file_read received a null pointer for file struct");
        return 0;
    }
    if (!buff) {
        KLOG_ERROR("io_fs_file_read received a null pointer for buff");
        return 0;
    }

    // TODO include pointer
    if (!(file->flags & IO_FS_FLAG_READ)) {
        KLOG_ERROR("io_fs_file_read called on file without read flag");
        return 0;
    }

    return file->file_read_fn(file->fs_data, file->file_data, buff, count);
}

size_t io_fs_file_write(io_fs_file_t * file, const char * buff, size_t count) {
    if (!file) {
        KLOG_ERROR("io_fs_file_write received a null pointer for file struct");
        return 0;
    }
    if (buff) {
        KLOG_ERROR("io_fs_file_write received a null pointer for buff");
        return 0;
    }

    // TODO include pointer
    if (!(file->flags & IO_FS_FLAG_WRITE)) {
        KLOG_ERROR("io_fs_file_write called on file without write flag");
        return 0;
    }

    return file->file_write_fn(file->fs_data, file->file_data, buff, count);
}

int io_fs_file_seek(io_fs_file_t * file, int offset, int origin) {
    if (!file) {
        KLOG_ERROR("io_fs_file_seek received a null pointer for file");
        return -1;
    }

    return file->file_seek_fn(file->fs_data, file->file_data, offset, origin);
}

int io_fs_file_tell(io_fs_file_t * file) {
    if (!file) {
        KLOG_ERROR("io_fs_file_tell received a null pointer for file");
        return -1;
    }

    return file->file_tell_fn(file->fs_data, file->file_data);
}
