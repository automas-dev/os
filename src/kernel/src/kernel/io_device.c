#define KLOG_SERVICE "KERNEL/IO_DEVICE"

#include "kernel/io/device.h"
#include "kernel/logs.h"

size_t io_device_read(io_device_t * dev, char * buff, size_t count, size_t pos) {
    if (!dev) {
        KLOG_WARNING("io_device_read received a null pointer for the device struct");
        return 0;
    }

    if (!(dev->flags & IO_DEVICE_FLAG_READ)) {
        KLOG_WARNING("io_device_read called on device without read flag");
        return 0;
    }

    if (!dev->read_fn) {
        KLOG_WARNING("io_device_read called on device with no read function");
        return 0;
    }

    return dev->read_fn(dev->device_data, buff, count, pos);
}

size_t io_device_write(io_device_t * dev, const char * buff, size_t count, size_t pos) {
    if (!dev) {
        KLOG_WARNING("io_device_write received a null pointer for the device struct");
        return 0;
    }

    if (!(dev->flags & IO_DEVICE_FLAG_WRITE)) {
        KLOG_WARNING("io_device_write called on device without write flag");
        return 0;
    }

    if (!dev->write_fn) {
        KLOG_WARNING("io_device_write called on device with no write function");
        return 0;
    }

    return dev->write_fn(dev->device_data, buff, count, pos);
}

size_t io_device_size(io_device_t * dev) {
    if (!dev) {
        KLOG_WARNING("io_device_size received a null pointer for the device struct");
        return 0;
    }

    if (!(dev->flags & IO_DEVICE_FLAG_SIZED)) {
        KLOG_WARNING("io_device_size called on device without sized flag");
        return 0;
    }

    if (!dev->size_fn) {
        KLOG_WARNING("io_device_size called on device with no size function");
        return 0;
    }

    return dev->size_fn(dev->device_data);
}

int io_device_close(io_device_t * dev) {
    if (!dev) {
        KLOG_WARNING("io_device_close received a null pointer for the device struct");
        return -1;
    }

    if (!dev->close_fn) {
        KLOG_WARNING("io_device_close called on device with no close function");
        return 0;
    }

    return dev->close_fn(dev->device_data);
}
