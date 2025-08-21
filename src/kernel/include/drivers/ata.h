#ifndef DRIVER_ATA_H
#define DRIVER_ATA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATA_SECTOR_WORDS 256
#define ATA_SECTOR_BYTES (ATA_SECTOR_WORDS * 2)

typedef struct _ata ata_t;

void init_ata();

ata_t * ata_open(uint8_t id);
void    ata_close(ata_t * disk);

size_t ata_size(ata_t * disk);
size_t ata_sector_count(ata_t * disk);
bool   ata_status(ata_t * disk);

// Read / write whole sector block (read / write bytes in file system driver)
// buff must be of size sect_count * ATA_SECTOR_BYTES
size_t ata_sect_read(ata_t * disk, uint8_t * buff, size_t sect_count, uint32_t lba);
size_t ata_sect_write(ata_t * disk, uint8_t * buff, size_t sect_count, uint32_t lba);

#endif // DRIVER_ATA_H
