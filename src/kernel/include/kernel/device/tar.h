#ifndef KERNEL_DEVICE_TAR_H
#define KERNEL_DEVICE_TAR_H

#include "kernel/io_fs.h"

io_fs_t * io_fs_tar_open(io_device_t * device);
void      io_fs_tar_close(io_fs_t * fs);

#endif // KERNEL_DEVICE_TAR_H
