#define KLOG_SERVICE "KERNEL/DEVICE/TAR"

#include "kernel/device/tar.h"

#include "drivers/tar.h"
#include "kernel/logs.h"
#include "kernel/memory.h"
#include "libc/string.h"

typedef struct _io_fs_tar {
    io_device_t * device;
} io_fs_tar_t;

static int           _tar_close(void * fs_data);
static io_device_t * _tar_file_open(void * fs_data, const char * path, const char * mode);
static int           _tar_file_close(void * file_data);
static int           _tar_file_stat(void * fs_data, const char * path, io_fs_stat_t * stat_out);
static size_t        _tar_file_read(void * file_data, char * buff, size_t size, size_t pos);
static size_t        _tar_file_size(void * file_data);

io_fs_t * io_fs_tar_open(io_device_t * device) {
    tar_fs_t * tar = tar_open(device);

    if (!tar) {
        KLOG_DEBUG("Failed to open tar fs");
        return 0;
    }

    KLOG_TRACE("Tar FS opened");

    io_fs_t * fs = kmalloc(sizeof(io_fs_t));
    if (!fs) {
        KLOG_ERROR("Failed to allocate memory for ata device");
        tar_close(tar);
        return 0;
    }

    kmemset(fs, 0, sizeof(io_fs_t));

    fs->dev = device;

    fs->close_fn = _tar_close;

    fs->file_open_fn = _tar_file_open;
    fs->file_stat_fn = _tar_file_stat;

    fs->fs_data = tar;

    return fs;
}

static int _tar_close(void * fs_data) {
    if (!fs_data) {
        KLOG_WARNING("Tried to free tar fs with null fs data");
        return 1;
    }

    tar_close(fs_data);
    return 0;
}

static io_device_t * _tar_file_open(void * fs_data, const char * path, const char * mode) {
    if (!fs_data) {
        KLOG_WARNING("Tried to open tar file with null fs_data");
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

    tar_fs_file_t * tar_file = tar_file_open(fs_data, path);
    if (!tar_file) {
        KLOG_WARNING("Failed to open file %s in mode %s", path, mode);
        return 0;
    }

    io_device_t * file = kmalloc(sizeof(io_device_t));
    if (!file) {
        KLOG_ERROR("Failed to allocate memory for tar file device");
        return 0;
    }

    kmemset(file, 0, sizeof(io_device_t));

    file->flags = IO_DEVICE_FLAG_READ | IO_DEVICE_FLAG_SIZED;

    file->read_fn = _tar_file_read;
    file->size_fn = _tar_file_size;
    file->close_fn = _tar_file_close;

    file->device_data = tar_file;

    while (*mode) {
        switch (*mode) {
            case 'r':
                KLOG_TRACE("File has read flag");
                break;
            case 'w':
                KLOG_TRACE("File has write flag");
                // TODO return error
                break;
            // TODO handle append mode
            case 'a':
                KLOG_TRACE("File has append flag");
                // TODO return error
                break;
            default:
                KLOG_TRACE("Unknown flag %c", *mode);
                break;
        }
        mode++;
    }

    return file;
}

static int _tar_file_close(void * file_data) {
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

static size_t _tar_file_read(void * file_data, char * buff, size_t size, size_t pos) {
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

    tar_fs_file_t * file = file_data;

    // TODO replace with pos arg to read fn
    tar_file_seek(file, pos, TAR_SEEK_ORIGIN_START);
    return tar_file_read(file, buff, size);
}

static size_t _tar_file_size(void * file_data) {
    if (!file_data) {
        KLOG_WARNING("Tried to read tar file with null file data");
        return 0;
    }

    tar_fs_file_t * file = file_data;

    return tar_file_size(file);
}
