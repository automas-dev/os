#ifndef KERNEL_DEVICE_SCREEN_H
#define KERNEL_DEVICE_SCREEN_H

#include <stddef.h>

#include "drivers/vga.h"
#include "kernel/io/device.h"

io_device_t * io_device_screen_open();

int io_device_screen_write_raw(int handle, const char * buff, size_t size, size_t pos);

#endif // KERNEL_DEVICE_SCREEN_H
