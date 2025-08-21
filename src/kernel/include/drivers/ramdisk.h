#ifndef DRIVER_RAMDISK_H
#define DRIVER_RAMDISK_H

#include <stddef.h>
#include <stdint.h>

#define RAMDISK_MAX 8

typedef struct _ramdisk ramdisk_t;

int ramdisk_create(size_t size);

ramdisk_t * ramdisk_open(int id);
void        ramdisk_close(ramdisk_t * rdisk);

size_t ramdisk_size(ramdisk_t * rdisk);

size_t ramdisk_read(ramdisk_t * rdisk, uint8_t * buff, size_t count, size_t pos);
size_t ramdisk_write(ramdisk_t * rdisk, uint8_t * buff, size_t count, size_t pos);

#endif // DRIVER_RAMDISK_H
