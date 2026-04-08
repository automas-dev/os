#ifndef KERNEL_DEVICE_ATA_H
#define KERNEL_DEVICE_ATA_H

#include <stddef.h>
#include <stdint.h>

#include "kernel/device/io.h"

io_device_t * device_ata_open(uint8_t id);
void          device_ata_close(io_device_t * device);

#endif // KERNEL_DEVICE_ATA_H
