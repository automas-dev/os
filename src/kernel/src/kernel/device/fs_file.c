#include "kernel/device/fs_file.h"

#include "kernel.h"
#include "kernel/io_fs.h"
#include "kernel/memory.h"
#include "libc/string.h"

static size_t _device_fs_file_read(void * device_data, char * buff, size_t size, size_t pos);
static size_t _device_fs_file_size(void * device_data);

io_device_t * device_fs_file_open(const char * path, const char * mode) {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = IO_DEVICE_FLAG_READ | IO_DEVICE_FLAG_SIZED;

        dev->read_fn = _device_fs_file_read;
        dev->size_fn = _device_fs_file_size;

        io_fs_file_t * file = io_fs_file_open(kernel_get_fs(), path, mode);
        if (!file) {
            kfree(dev);
            return 0;
        }

        dev->device_data = file;
    }
    return dev;
}

void device_fs_file_close(io_device_t * d) {
    if (d) {
        kfree(d);
    }
}

static size_t __size(void * ptr) {
    if (!ptr) {
        return 0;
    }

    return tar_file_size(ptr);
}

static size_t _device_fs_file_read(void * device_data, char * buff, size_t size, size_t pos) {
    if (!device_data) {
        // TODO log
        return 0;
    }
    if (!buff) {
        // TODO log
        return 0;
    }

    io_fs_file_t * file = device_data;

    if (io_fs_file_seek(file, pos, FILE_SEEK_ORIGIN_START)) {
        // TODO log
        return 0;
    }

    return io_fs_file_read(file, buff, size);
}

static size_t _device_fs_file_size(void * device_data) {
    if (!device_data) {
        // TODO log
        return 0;
    }
    io_fs_file_t * file = device_data;

    if (io_fs_file_seek(file, 0, FILE_SEEK_ORIGIN_END)) {
        // TODO log
        return 0;
    }

    int size = io_fs_file_tell(file);
    if (size < 0) {
        return 0;
    }
    return size;
}
