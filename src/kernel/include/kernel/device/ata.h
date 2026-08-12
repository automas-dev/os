#ifndef KERNEL_DEVICE_ATA_H
#define KERNEL_DEVICE_ATA_H

#include <stddef.h>
#include <stdint.h>

#include "kernel/io_device.h"

io_device_t * io_device_ata_open(uint8_t id);

#endif // KERNEL_DEVICE_ATA_H
