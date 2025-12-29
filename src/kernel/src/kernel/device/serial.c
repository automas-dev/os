#include "kernel/device/serial.h"

#include "drivers/serial.h"
#include "kernel/memory.h"
#include "libc/string.h"

static size_t _serial_write(void * ptr, const char * buff, size_t size, size_t pos);

io_device_t * device_serial_open() {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = DEVICE_IO_FLAG_WRITE;

        dev->read_fn  = 0;
        dev->write_fn = _serial_write;
    }
    return dev;
}

void device_serial_close(io_device_t * device) {
    if (device) {
        kfree(device);
    }
}

// ptr and pos not used
static size_t _serial_write(void * device_data, const char * buff, size_t size, size_t pos) {
    serial_write(SERIAL_PORT_COM1, buff, size);
    return size;
}

// handle and pos not used
int device_serial_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    serial_write(SERIAL_PORT_COM1, buff, size);
    return size;
}
