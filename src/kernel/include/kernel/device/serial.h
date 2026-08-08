#ifndef KERNEL_DEVICE_SERIAL_H
#define KERNEL_DEVICE_SERIAL_H

#include <stddef.h>

#include "kernel/io.h"

io_device_t * device_serial_open();

int device_serial_write_raw(int handle, const char * buff, size_t size, size_t pos);

#endif // KERNEL_DEVICE_SERIAL_H
