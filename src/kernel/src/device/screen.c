#include "kernel/device/screen.h"

#include "kernel/memory.h"

static size_t _vga_write(void * ptr, const char * buff, size_t size, size_t pos);

io_device_t * device_screen_open() {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        dev->flags = DEVICE_IO_FLAG_WRITE;

        dev->read_fn  = 0;
        dev->write_fn = _vga_write;
    }
    return dev;
}

void device_screen_close(io_device_t * d) {
    if (d) {
        kfree(d);
    }
}

// ptr and pos not used
static size_t _vga_write(void * ptr, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}

// handle and pos not used
int device_screen_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}
