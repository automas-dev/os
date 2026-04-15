#define KLOG_SERVICE "KERNEL/DEVICE/ATA"

#include "kernel/device/ata.h"

#include "drivers/ata.h"
#include "kernel/logs.h"
#include "kernel/memory.h"
#include "libc/string.h"

typedef struct _ata_device {
    ata_t * ata;
    char    buffer[ATA_SECTOR_BYTES];
} ata_device_t;

static size_t _ata_read(void * device_data, char * buff, size_t size, size_t pos);
static size_t _ata_write(void * device_data, const char * buff, size_t size, size_t pos);
static size_t _ata_size(void * device_data);

io_device_t * device_ata_open(uint8_t id) {
    ata_t * ata_dev = ata_open(id);
    if (!ata_dev) {
        KLOG_WARNING("Failed to open ata driver for device id %u", id);
        return 0;
    }

    io_device_t * dev = kmalloc(sizeof(io_device_t));
    if (!dev) {
        KLOG_ERROR("Failed to allocate memory for ata device");
        ata_close(ata_dev);
        return 0;
    }

    kmemset(dev, 0, sizeof(io_device_t));

    dev->flags = IO_DEVICE_FLAG_READ | IO_DEVICE_FLAG_WRITE | IO_DEVICE_FLAG_SIZED;

    dev->read_fn  = _ata_read;
    dev->write_fn = _ata_write;
    dev->size_fn  = _ata_size;

    dev->device_data = ata_dev;

    return dev;
}

void device_ata_close(io_device_t * device) {
    if (!device) {
        KLOG_WARNING("Tried to free null ata device");
        return;
    }
    if (!device->device_data) {
        KLOG_WARNING("Tried to free ata device with null device data");
        return;
    }

    ata_close(device->device_data);
    kfree(device);
}

static size_t _ata_read(void * device_data, char * buff, size_t size, size_t pos) {
    ata_t * ata        = device_data;
    size_t  read_total = 0;
    char    buffer[ATA_SECTOR_BYTES];

    size_t disk_size = ata_size(ata);
    if (size + pos > disk_size) {
        KLOG_DEBUG("Shrinking read size %u to available %u after pos %u", size, disk_size, pos);
        size = disk_size - pos;
    }

    KLOG_TRACE("Read %u bytes from %u", size, pos);

    while (size) {
        uint32_t lba = pos / ATA_SECTOR_BYTES;
        if (!ata_sect_read(ata, buffer, 1, lba)) {
            KLOG_WARNING("ATA device failed to read 1 sector from lba %u", lba);
            break;
        }

        size_t sector_pos = pos % ATA_SECTOR_BYTES;
        size_t to_read    = ATA_SECTOR_BYTES - sector_pos;
        if (size < to_read) {
            to_read = size;
        }

        kmemcpy(&buff[read_total], &buffer[sector_pos], to_read);

        pos += to_read;
        size -= to_read;
        read_total += to_read;
    }

    return read_total;
}

static size_t _ata_write(void * device_data, const char * buff, size_t size, size_t pos) {
    ata_t * ata         = device_data;
    size_t  write_total = 0;

    size_t disk_size = ata_size(ata);
    if (size + pos > disk_size) {
        KLOG_DEBUG("Shrinking write size %u to available %u after pos %u", size, disk_size, pos);
        size = disk_size - pos;
    }

    char buffer[ATA_SECTOR_BYTES];

    KLOG_TRACE("Write %u bytes from %u", size, pos);

    if (pos % ATA_SECTOR_BYTES) {
        uint32_t lba = pos / ATA_SECTOR_BYTES;

        if (!ata_sect_read(ata, buffer, 1, lba)) {
            KLOG_WARNING("ATA device failed to read 1 sector from lba %u", lba);
            return 0;
        }

        size_t sector_pos = pos % ATA_SECTOR_BYTES;
        size_t to_write   = ATA_SECTOR_BYTES - sector_pos;
        if (size < to_write) {
            to_write = size;
        }

        kmemcpy(&buffer[sector_pos], &buff[write_total], to_write);

        if (!ata_sect_write(ata, buffer, 1, lba)) {
            KLOG_WARNING("ATA device failed to write 1 sector from lba %u", lba);
            return 0;
        }

        pos += to_write;
        size -= to_write;
        write_total += to_write;

        if (!size) {
            return write_total;
        }
    }

    while (size >= ATA_SECTOR_BYTES) {
        uint32_t lba = pos / ATA_SECTOR_BYTES;

        if (!ata_sect_write(ata, &buff[write_total], 1, lba)) {
            KLOG_WARNING("ATA device failed to write 1 sector from lba %u", lba);
            return write_total;
        }

        pos += ATA_SECTOR_BYTES;
        size -= ATA_SECTOR_BYTES;
        write_total += ATA_SECTOR_BYTES;
    }

    if (size) {
        uint32_t lba = pos / ATA_SECTOR_BYTES;

        if (!ata_sect_read(ata, buffer, 1, lba)) {
            KLOG_WARNING("ATA device failed to read 1 sector from lba %u", lba);
            return write_total;
        }

        size_t to_write = ATA_SECTOR_BYTES;
        if (size < to_write) {
            to_write = size;
        }

        kmemcpy(buffer, &buff[write_total], to_write);

        if (!ata_sect_write(ata, buffer, 1, lba)) {
            KLOG_WARNING("ATA device failed to write 1 sector from lba %u", lba);
            return 0;
        }

        write_total += to_write;
    }

    return write_total;
}

static size_t _ata_size(void * device_data) {
    ata_t * ata = device_data;
    return ata_size(ata);
}
