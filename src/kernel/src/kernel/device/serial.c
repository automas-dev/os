#include "kernel/device/serial.h"

#include "drivers/serial.h"
#include "kernel/memory.h"
#include "libc/string.h"

// WARNING the serial device backs kernel logging, so do not log in this file or
// you will get an infinite loop.

typedef struct _serial_device {
    uint16_t port;
} serial_device_t;

static int    _serial_close(void * device_data);
static size_t _serial_read(void * device_data, char * buff, size_t size, size_t pos);
static size_t _serial_write(void * device_data, const char * buff, size_t size, size_t pos);

io_device_t * io_device_serial_open(uint16_t port) {
    serial_device_t * serial = kmalloc(sizeof(serial_device_t));
    if (!serial) {
        return 0;
    }

    serial->port = port;

    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (!dev) {
        kfree(serial);
        return 0;
    }

    kmemset(dev, 0, sizeof(io_device_t));

    dev->flags = IO_DEVICE_FLAG_READ | IO_DEVICE_FLAG_WRITE;

    dev->close_fn = _serial_close;
    dev->read_fn  = _serial_read;
    dev->write_fn = _serial_write;

    dev->device_data = serial;

    return dev;
}

// handle and pos not used
int io_device_serial_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    return serial_write(SERIAL_PORT_COM1, buff, size);
}

static int _serial_close(void * device_data) {
    if (!device_data) {
        return 1;
    }

    kfree(device_data);
    return 0;
}

// pos not used
static size_t _serial_read(void * device_data, char * buff, size_t size, size_t pos) {
    if (!device_data || !buff) {
        return 0;
    }

    serial_device_t * serial = device_data;
    return serial_read(serial->port, buff, size);
}

// pos not used
static size_t _serial_write(void * device_data, const char * buff, size_t size, size_t pos) {
    if (!device_data || !buff) {
        return 0;
    }

    serial_device_t * serial = device_data;
    return serial_write(serial->port, buff, size);
}
