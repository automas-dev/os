#ifndef KERNEL_DEVICE_SCREEN_H
#define KERNEL_DEVICE_SCREEN_H

#include <stddef.h>

#include "kernel/device/io.h"
#include "vga.h"

io_device_t * device_screen_open();
void          device_screen_close(io_device_t *);

int device_screen_write_raw(int, const char * buff, size_t size, size_t pos);

#endif // KERNEL_DEVICE_SCREEN_H
