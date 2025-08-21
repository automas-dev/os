#ifndef DRIVER_DISK_H
#define DRIVER_DISK_H

#include <stddef.h>
#include <stdint.h>

typedef struct _disk disk_t;

enum DISK_DRIVER {
    DISK_DRIVER_ATA = 0,
    DISK_DRIVER_FLOPPY,
    DISK_DRIVER_RAM_DISK,
};

disk_t * disk_open(int id, enum DISK_DRIVER driver);
void     disk_close(disk_t * disk);

size_t disk_size(disk_t * disk);

size_t disk_read(disk_t * disk, uint8_t * buff, size_t count, size_t pos);
size_t disk_write(disk_t * disk, uint8_t * buff, size_t count, size_t pos);

#endif // DRIVER_DISK_H
