#ifndef KERNEL_DEVICE_IO_H
#define KERNEL_DEVICE_IO_H

#include <stddef.h>

enum DEVICE_IO_FLAG {
    DEVICE_IO_FLAG_READ  = 0x1,
    DEVICE_IO_FLAG_WRITE = 0x2,
    DEVICE_IO_FLAG_SIZED = 0x4,
};

typedef size_t (*device_io_read_t)(void * data, char * buff, size_t count, size_t pos);
typedef size_t (*device_io_write_t)(void * data, const char * buff, size_t count, size_t pos);
typedef size_t (*device_io_size_t)(void * data);
// typedef size_t (*device_io_close_t)(io_device_t * data);

typedef struct _io_device {
    int flags;

    device_io_read_t  read_fn;
    device_io_write_t write_fn;
    device_io_size_t  size_fn;
    // device_io_close_t close_fn;

    void * data;
} io_device_t;

#endif // KERNEL_DEVICE_IO_H
