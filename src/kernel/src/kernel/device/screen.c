#include "kernel/device/screen.h"

#include "kernel/memory.h"
#include "libc/string.h"

static size_t _vga_write(void * ptr, const char * buff, size_t size, size_t pos);

io_device_t * io_device_screen_open() {
    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (dev) {
        kmemset(dev, 0, sizeof(io_device_t));

        dev->flags = IO_DEVICE_FLAG_WRITE;

        dev->write_fn = _vga_write;
    }
    return dev;
}

void device_screen_close(io_device_t * device) {
    if (device) {
        kfree(device);
    }
}

// ptr and pos not used
static size_t _vga_write(void * device_data, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}

// handle and pos not used
int io_device_screen_write_raw(int handle, const char * buff, size_t size, size_t pos) {
    return vga_write(buff, size);
}
