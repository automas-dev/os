#include "kernel/io.h"

size_t io_device_read(io_device_t * dev, char * buff, size_t count, size_t pos) {
    if (!dev) {
        // TODO log
        return 0;
    }

    return dev->read_fn(dev->device_data, buff, count, pos);
}

size_t io_device_write(io_device_t * dev, const char * buff, size_t count, size_t pos) {
    if (!dev) {
        // TODO log
        return 0;
    }

    return dev->write_fn(dev->device_data, buff, count, pos);
}

size_t io_device_size(io_device_t * dev) {
    if (!dev) {
        // TODO log
        return 0;
    }

    return dev->size_fn(dev->device_data);
}

int io_device_close(io_device_t * dev) {
    if (!dev) {
        // TODO log
        return -1;
    }

    return dev->close_fn(dev->device_data);
}
