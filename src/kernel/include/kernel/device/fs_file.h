#ifndef KERNEL_DEVICE_FS_FILE_H
#define KERNEL_DEVICE_FS_FILE_H

#include <stddef.h>

#include "kernel/io_fs.h"

/// TODO remove
/// @deprecated
io_device_t * device_fs_file_open(const char * path, const char * mode);

#endif // KERNEL_DEVICE_FS_FILE_H
