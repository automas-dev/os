#include "kernel/device/fs_file.h"

#include "drivers/tar.h"
#include "kernel.h"
#include "kernel/memory.h"
#include "libc/string.h"

static size_t __size(void * ptr);

io_device_t * device_fs_file_open(const char * path, const char * mode) {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = DEVICE_IO_FLAG_WRITE | DEVICE_IO_FLAG_READ | DEVICE_IO_FLAG_SIZED;

        dev->read_fn  = (device_io_read_t)tar_file_read;
        dev->write_fn = (device_io_write_t)tar_file_read;
        dev->size_fn  = __size;

        tar_fs_file_t * file = tar_file_open(kernel_get_tar(), path);
        if (!file) {
            kfree(dev);
            return 0;
        }

        dev->data = file;
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
