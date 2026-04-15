#include "kernel/device/screen.h"

#include "kernel/memory.h"
#include "libc/string.h"

static size_t _vga_read(void * ptr, char * buff, size_t size, size_t pos);
static size_t _vga_write(void * ptr, const char * buff, size_t size, size_t pos);
static size_t _vga_size(void * ptr);

io_device_t * device_screen_open() {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = IO_DEVICE_FLAG_WRITE;

        dev->read_fn  = _vga_read;
        dev->write_fn = _vga_write;
        dev->size_fn  = _vga_size;
    }
    return dev;
}

void device_screen_close(io_device_t * device) {
    if (device) {
        kfree(device);
    }
}

// ptr and pos not used
static size_t _vga_read(void * device_data, char * buff, size_t size, size_t pos) {
    return 0;
}

// ptr and pos not used
static size_t _vga_write(void * device_data, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}

// handle and pos not used
int device_screen_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}

static size_t _vga_size(void * ptr) {
    return 0;
}
