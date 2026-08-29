#include "kernel/device/screen.h"

#include "kernel/memory.h"
#include "libc/string.h"

static int    _vga_close(void * device_data);
static size_t _vga_write(void * device_data, const char * buff, size_t size, size_t pos);

io_device_t * io_device_screen_open() {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = IO_DEVICE_FLAG_WRITE;

        dev->close_fn = _vga_close;

        dev->write_fn = _vga_write;
    }
    return dev;
}

static int _vga_close(void * device_data) {
    // Nothing to free
    return 0;
}

// device_data and pos not used
static size_t _vga_write(void * device_data, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}

// handle and pos not used
int io_device_screen_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}
