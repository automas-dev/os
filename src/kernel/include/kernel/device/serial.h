#ifndef KERNEL_DEVICE_SERIAL_H
#define KERNEL_DEVICE_SERIAL_H

#include <stddef.h>

#include "kernel/io_device.h"

io_device_t * io_device_serial_open();

int io_device_serial_write_raw(int handle, const char * buff, size_t size, size_t pos);

#endif // KERNEL_DEVICE_SERIAL_H
