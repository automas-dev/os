#ifndef KERNEL_DEVICE_FS_FILE_H
#define KERNEL_DEVICE_FS_FILE_H

#include <stddef.h>

#include "kernel/device/io.h"

io_device_t * device_fs_file_open(const char * path, const char * mode);
void          device_fs_file_close(io_device_t *);

#endif // KERNEL_DEVICE_FS_FILE_H
