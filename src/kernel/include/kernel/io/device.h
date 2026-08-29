#ifndef KERNEL_IO_DEVICE_H
#define KERNEL_IO_DEVICE_H

#include <stddef.h>

enum IO_DEVICE_FLAG {
    IO_DEVICE_FLAG_READ  = 0x1,
    IO_DEVICE_FLAG_WRITE = 0x2,
    IO_DEVICE_FLAG_SIZED = 0x4,
};

typedef size_t (*io_device_read_t)(void * device_data, char * buff, size_t count, size_t pos);
typedef size_t (*io_device_write_t)(void * device_data, const char * buff, size_t count, size_t pos);
typedef size_t (*io_device_size_t)(void * device_data);
typedef int (*io_device_close_t)(void * device_data);

typedef struct _io_device {
    int flags;

    // Required
    io_device_close_t close_fn;

    // Optional (availability based on flags)
    io_device_read_t  read_fn;
    io_device_write_t write_fn;
    io_device_size_t  size_fn;

    void * device_data;
} io_device_t;

size_t io_device_read(io_device_t * dev, char * buff, size_t count, size_t pos);
size_t io_device_write(io_device_t * dev, const char * buff, size_t count, size_t pos);
size_t io_device_size(io_device_t * dev);
int    io_device_close(io_device_t * dev);

#endif // KERNEL_IO_DEVICE_H
