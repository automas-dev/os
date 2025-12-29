#include "drivers/disk.h"

#include "drivers/ata.h"
#include "drivers/ramdisk.h"
#include "kernel.h"
#include "libc/memory.h"
#include "libc/string.h"

#define DISK_BUFFER_SIZE 4096

typedef size_t (*disk_io)(disk_t * disk, uint8_t * buff, size_t count, size_t pos);

struct _disk {
    enum DISK_DRIVER driver;

    char * buff;
    size_t buff_size;

    union {
        ata_t * ata;
        // TODO floppy
        ramdisk_t * rdisk;
    } device;

    size_t  size;
    disk_io fn_read;
    disk_io fn_write;
};

static size_t disk_ata_read(disk_t * disk, uint8_t * buff, size_t count, size_t pos);
static size_t disk_ata_write(disk_t * disk, uint8_t * buff, size_t count, size_t pos);

static size_t disk_rdisk_read(disk_t * disk, uint8_t * buff, size_t count, size_t pos);
static size_t disk_rdisk_write(disk_t * disk, uint8_t * buff, size_t count, size_t pos);

disk_t * disk_open(int id, enum DISK_DRIVER driver) {
    disk_t * disk = kmalloc(sizeof(disk_t));
    if (disk) {
        disk->driver = driver;

        disk->buff_size = DISK_BUFFER_SIZE;
        disk->buff      = kmalloc(disk->buff_size);
        if (!disk->buff) {
            kfree(disk);
            return 0;
        }

        switch (driver) {
            case DISK_DRIVER_ATA: {
                disk->device.ata = ata_open(id);
                if (!disk->device.ata) {
                    kfree(disk->buff);
                    kfree(disk);
                    return 0;
                }

                disk->size     = ata_size(disk->device.ata);
                disk->fn_read  = disk_ata_read;
                disk->fn_write = disk_ata_write;
            } break;
            case DISK_DRIVER_FLOPPY: {
                // TODO open floppy driver
                kfree(disk->buff);
                kfree(disk);
                return 0;
            } break;
            case DISK_DRIVER_RAM_DISK: {
                disk->device.rdisk = ramdisk_open(id);
                if (!disk->device.rdisk) {
                    kfree(disk->buff);
                    kfree(disk);
                    return 0;
                }
                disk->size     = ramdisk_size(disk->device.rdisk);
                disk->fn_read  = disk_rdisk_read;
                disk->fn_write = disk_rdisk_write;
            } break;
            default: {
                return 0;
            } break;
        }
    }
    return disk;
}

void disk_close(disk_t * disk) {
    if (disk) {
        switch (disk->driver) {
            case DISK_DRIVER_ATA: {
                ata_close(disk->device.ata);
            } break;
            case DISK_DRIVER_FLOPPY: {

            } break;
            case DISK_DRIVER_RAM_DISK: {

            } break;
            default: {

            } break;
        }
        kfree(disk->buff);
        kfree(disk);
    }
}

size_t disk_size(disk_t * disk) {
    if (!disk) {
        return 0;
    }
    return disk->size;
}

size_t disk_read(disk_t * disk, uint8_t * buff, size_t count, size_t pos) {
    if (!disk || !buff) {
        return 0;
    }

    size_t steps = count / disk->buff_size;
    if (count % disk->buff_size) {
        steps++;
    }

    size_t o_len = 0;
    for (size_t i = 0; i < steps; i++) {
        size_t offset = i * disk->buff_size;
        o_len += disk->fn_read(disk, buff + offset, count - offset, pos + offset);
    }
    return o_len;
}

size_t disk_write(disk_t * disk, uint8_t * buff, size_t count, size_t pos) {
    if (!disk || !buff) {
        return 0;
    }

    size_t steps = count / disk->buff_size;
    if (count % disk->buff_size) {
        steps++;
    }

    size_t o_len = 0;
    for (size_t i = 0; i < steps; i++) {
        size_t offset = i * disk->buff_size;
        o_len += disk->fn_write(disk, buff + offset, count - offset, pos + offset);
    }
    return o_len;
}

static size_t disk_ata_read(disk_t * disk, uint8_t * buff, size_t count, size_t pos) {
    if (!disk || !buff) {
        return 0;
    }

    if (count > disk->buff_size) {
        count = disk->buff_size;
    }

    if (disk->size - pos < count) {
        count = disk->size - pos;
    }

    size_t lba          = pos / ATA_SECTOR_BYTES;
    size_t sect_to_read = count / ATA_SECTOR_BYTES;

    if (count % ATA_SECTOR_BYTES) {
        sect_to_read++;
    }

    if (ata_sect_read(disk->device.ata, disk->buff, sect_to_read, lba)
        != sect_to_read) {
        return 0;
    }

    size_t pos_in_buff = pos % ATA_SECTOR_BYTES;
    kmemcpy(buff, disk->buff + pos_in_buff, count);
    return count;
}

static size_t disk_ata_write(disk_t * disk, uint8_t * buff, size_t count, size_t pos) {
    if (!disk || !buff) {
        return 0;
    }

    if (count > disk->buff_size) {
        count = disk->buff_size;
    }

    if (disk->size - pos < count) {
        count = disk->size - pos;
    }

    size_t lba           = pos / ATA_SECTOR_BYTES;
    size_t sect_to_write = count / ATA_SECTOR_BYTES;

    if (count % ATA_SECTOR_BYTES) {
        if (!ata_sect_read(disk->device.ata, disk->buff, 1, lba)) {
            return 0;
        }
        sect_to_write++;
    }

    kmemcpy(disk->buff, buff, count);

    if (ata_sect_write(disk->device.ata, disk->buff, sect_to_write, lba)
        != sect_to_write) {
        return 0;
    }

    return count;
}

static size_t disk_rdisk_read(disk_t * disk, uint8_t * buff, size_t count, size_t pos) {
    if (!disk || !buff) {
        return 0;
    }

    if (count > disk->buff_size) {
        count = disk->buff_size;
    }

    if (disk->size - pos < count) {
        count = disk->size - pos;
    }

    size_t o_len = ramdisk_read(disk->device.rdisk, buff, count, pos);
    return o_len;
}

static size_t disk_rdisk_write(disk_t * disk, uint8_t * buff, size_t count, size_t pos) {
    if (!disk || !buff) {
        return 0;
    }

    if (count > disk->buff_size) {
        count = disk->buff_size;
    }

    if (disk->size - pos < count) {
        count = disk->size - pos;
    }

    size_t o_len = ramdisk_write(disk->device.rdisk, buff, count, pos);
    return o_len;
}
