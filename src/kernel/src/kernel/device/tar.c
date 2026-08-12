#define KLOG_SERVICE "KERNEL/DEVICE/TAR"

#include "kernel/device/tar.h"

#include "drivers/tar.h"
#include "kernel/logs.h"
#include "kernel/memory.h"
#include "libc/string.h"

typedef struct _io_fs_tar {
    io_device_t * device;
} io_fs_tar_t;

static void * _tar_file_open(void * fs_data, const char * path, const char * mode);
static int    _tar_file_close(void * fs_data, void * file_data);
static int    _tar_file_stat(void * fs_data, const char * path, io_fs_stat_t * stat_out);
static size_t _tar_file_read(void * fs_data, void * file_data, char * buff, size_t size);
static size_t _tar_file_write(void * fs_data, void * file_data, const char * buff, size_t size);
static int    _tar_file_seek(void * fs_data, void * file_data, int offset, int origin);
static size_t _tar_file_tell(void * fs_data, void * file_data);

io_fs_t * io_fs_tar_open(io_device_t * device) {
    tar_fs_t * tar = tar_open(device);

    if (!tar) {
        KLOG_WARNING("Failed to open tar fs");
        return 0;
    }

    io_fs_t * fs = kmalloc(sizeof(io_fs_t));
    if (!fs) {
        KLOG_ERROR("Failed to allocate memory for ata device");
        tar_close(tar);
        return 0;
    }

    kmemset(fs, 0, sizeof(io_fs_t));

    fs->dev = device;

    fs->file_open_fn  = _tar_file_open;
    fs->file_close_fn = _tar_file_close;
    fs->file_stat_fn  = _tar_file_stat;
    fs->file_read_fn  = _tar_file_read;
    fs->file_write_fn = _tar_file_write;
    fs->file_seek_fn  = _tar_file_seek;
    fs->file_tell_fn  = _tar_file_tell;

    fs->fs_data = tar;

    return fs;
}

// void io_fs_tar_close(io_fs_t * fs) {
//     if (!fs) {
//         KLOG_WARNING("Tried to free null tar fs");
//         return;
//     }
//     if (!fs->fs_data) {
//         KLOG_WARNING("Tried to free tar fs with null fs data");
//         return;
//     }

//     tar_close(fs->fs_data);

//     kfree(fs);
// }

static void * _tar_file_open(void * fs_data, const char * path, const char * mode) {
    if (!fs_data) {
        KLOG_WARNING("Tried to open tar file with null fs data");
        return 0;
    }
    if (!path) {
        KLOG_WARNING("Tried to open tar file with null path");
        return 0;
    }
    if (!mode) {
        KLOG_WARNING("Tried to open tar file with null mode");
        return 0;
    }

    tar_fs_t * tar = fs_data;

    tar_fs_file_t * file = tar_file_open(tar, path);
    if (!file) {
        KLOG_WARNING("Failed to open file %s in mode %u", path, mode);
        return 0;
    }

    return file;
}

static int _tar_file_close(void * fs_data, void * file_data) {
    if (!fs_data) {
        KLOG_WARNING("Tried to close tar file with null fs data");
        return -1;
    }
    if (!file_data) {
        KLOG_WARNING("Tried to close tar file with null file data");
        return -1;
    }

    tar_file_close(file_data);

    return 0;
}

static int _tar_file_stat(void * fs_data, const char * path, io_fs_stat_t * stat_out) {
    if (!fs_data) {
        KLOG_WARNING("Tried to stat tar file with null fs data");
        return -1;
    }
    if (!path) {
        KLOG_WARNING("Tried to stat tar file with null path");
        return -1;
    }
    if (!stat_out) {
        KLOG_WARNING("Tried to stat tar file into null stat struct");
        return -1;
    }

    tar_fs_t * tar = fs_data;
    tar_stat_t stat;

    if (!tar_stat_file(tar, path, &stat)) {
        KLOG_WARNING("Failed to stat tar file %s", path);
        return -1;
    }

    stat_out->group = stat.gid;
    stat_out->user  = stat.uid;
    stat_out->mode  = stat.mode;
    stat_out->ctime = stat.mtime;
    stat_out->ctime = stat.mtime;
    stat_out->atime = stat.mtime;
    stat_out->size  = stat.size;

    return 0;
}

static size_t _tar_file_read(void * fs_data, void * file_data, char * buff, size_t size) {
    if (!fs_data) {
        KLOG_WARNING("Tried to read tar file with null fs data");
        return 0;
    }
    if (!file_data) {
        KLOG_WARNING("Tried to read tar file with null file data");
        return 0;
    }
    if (!buff) {
        KLOG_WARNING("Tried to write tar file to null buffer");
        return 0;
    }
    if (!size) {
        KLOG_DEBUG("Read size is 0, returning early");
        return 0;
    }

    tar_fs_t *      tar  = fs_data;
    tar_fs_file_t * file = file_data;

    return tar_file_read(file, buff, size);
}

static size_t _tar_file_write(void * fs_data, void * file_data, const char * buff, size_t size) {
    return 0;
}

static int _tar_file_seek(void * fs_data, void * file_data, int offset, int origin) {
    if (!fs_data) {
        KLOG_WARNING("Tried to seek tar file with null fs data");
        return -1;
    }
    if (!file_data) {
        KLOG_WARNING("Tried to seek tar file with null file data");
        return -1;
    }
    if (origin < IO_FS_SEEK_ORIGIN_START || origin > IO_FS_SEEK_ORIGIN_END) {
        KLOG_WARNING("Tried to seek tar file with invalid origin %d", origin);
        return -1;
    }

    tar_fs_file_t * file = file_data;

    if (!tar_file_seek(file, offset, origin)) {
        KLOG_WARNING("Failed to seek offset %d from origin %u", offset, origin);
        return -1;
    }

    return 0;
}

static size_t _tar_file_tell(void * fs_data, void * file_data) {
    if (!fs_data) {
        KLOG_WARNING("Tried to tell tar file with null fs data");
        return 0;
    }
    if (!file_data) {
        KLOG_WARNING("Tried to tell tar file with null file data");
        return 0;
    }

    tar_fs_file_t * file = file_data;

    return tar_file_tell(file);
}
