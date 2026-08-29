#include "kernel/device/serial.h"

#include "drivers/serial.h"
#include "kernel/memory.h"
#include "libc/string.h"

static int    _serial_close(void * device_data);
static size_t _serial_read(void * device_data, char * buff, size_t size, size_t pos);
static size_t _serial_write(void * device_data, const char * buff, size_t size, size_t pos);
static size_t _serial_size(void * device_data);

io_device_t * io_device_serial_open() {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = IO_DEVICE_FLAG_WRITE;

        dev->close_fn = _serial_close;

        dev->read_fn  = _serial_read;
        dev->write_fn = _serial_write;
        dev->size_fn  = _serial_size;
    }
    return dev;
}

static int _serial_close(void * device_data) {
    // Nothing to free
    return 0;
}

// device_data and pos not used
static size_t _serial_read(void * device_data, char * buff, size_t size, size_t pos) {
    return 0;
}

// device_data and pos not used
static size_t _serial_write(void * device_data, const char * buff, size_t size, size_t pos) {
    serial_write(SERIAL_PORT_COM1, buff, size);
    return size;
}

// handle and pos not used
int io_device_serial_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    serial_write(SERIAL_PORT_COM1, buff, size);
    return size;
}

static size_t _serial_size(void * device_data) {
    return 0;
}
