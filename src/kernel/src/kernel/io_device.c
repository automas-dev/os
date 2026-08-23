#define KLOG_SERVICE "KERNEL/IO_DEVICE"

#include "kernel/io/device.h"
#include "kernel/logs.h"

// TODO should null ptr checks be here or in each read function?
size_t io_device_read(io_device_t * dev, char * buff, size_t count, size_t pos) {
    if (!dev) {
        KLOG_ERROR("io_device_read received a null pointer for the device struct");
        return 0;
    }
    if (!buff) {
        KLOG_ERROR("io_device_read received a null pointer for the buffer");
        return 0;
    }

    return dev->read_fn(dev->device_data, buff, count, pos);
}

size_t io_device_write(io_device_t * dev, const char * buff, size_t count, size_t pos) {
    if (!dev) {
        KLOG_ERROR("io_device_write received a null pointer for the device struct");
        return 0;
    }
    if (!buff) {
        KLOG_ERROR("io_device_write received a null pointer for the buffer");
        return 0;
    }

    return dev->write_fn(dev->device_data, buff, count, pos);
}

size_t io_device_size(io_device_t * dev) {
    if (!dev) {
        KLOG_ERROR("io_device_size received a null pointer for the device struct");
        return 0;
    }

    return dev->size_fn(dev->device_data);
}

int io_device_close(io_device_t * dev) {
    if (!dev) {
        KLOG_ERROR("io_device_close received a null pointer for the device struct");
        return -1;
    }

    return dev->close_fn(dev->device_data);
}
