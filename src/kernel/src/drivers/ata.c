#define KLOG_SERVICE "DRIVERS/ATA"

#include "drivers/ata.h"

#include "cpu/isr.h"
#include "cpu/ports.h"
#include "debug.h"
#include "drivers/rtc.h"
#include "kernel.h"
#include "kernel/logs.h"
#include "kernel/time.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"

// https://wiki.osdev.org/ATA_PIO_Mode

#define MAX_RETRY     5000
#define TIMEOUT_MS    1000
#define START_TIMEOUT uint32_t __timeout = time_ms() + TIMEOUT_MS;
#define TEST_TIMEOUT                                                                     \
    if (time_ms() > __timeout) {                                                         \
        KLOG_WARNING("Timeout occurred after %u (timeout is %u)", time_ms(), __timeout); \
        return 0;                                                                        \
    }
#define TEST_TIMEOUT_VOID                                                                \
    if (time_ms() > __timeout) {                                                         \
        KLOG_WARNING("Timeout occurred after %u (timeout is %u)", time_ms(), __timeout); \
        return;                                                                          \
    }

#define ATA_BUS_0_IO_BASE  0x1F0
#define ATA_BUS_0_CTL_BASE 0x3F6

enum ATA_IO {
    ATA_IO_DATA          = 0,                    // R/W
    ATA_IO_ERROR         = 1,                    // R
    ATA_IO_FEATURE       = 1,                    // W
    ATA_IO_SECTOR_COUNT  = 2,                    // R/W
    ATA_IO_SECTOR_NUMBER = 3,                    // R/W
    ATA_IO_LBA_LOW       = ATA_IO_SECTOR_NUMBER, // R/W
    ATA_IO_CYLINDER_LOW  = 4,                    // R/W
    ATA_IO_LBA_MID       = ATA_IO_CYLINDER_LOW,  // R/W
    ATA_IO_CYLINDER_HIGH = 5,                    // R/W
    ATA_IO_LBA_HIGH      = ATA_IO_CYLINDER_HIGH, // R/W
    ATA_IO_DRIVE_HEAD    = 6,                    // R/W
    ATA_IO_STATUS        = 7,                    // R
    ATA_IO_COMMAND       = 7,                    // W
};

enum ATA_CTL {
    ATA_CTL_ALT_STATUS = 0, // R
    ATA_CTL_CONTROL    = 0, // W
    ATA_CTL_ADDRESS    = 1, // R
};

enum ATA_ERROR_FLAG {
    ATA_ERROR_FLAG_AMNF  = 0x1,  // Address mark not found
    ATA_ERROR_FLAG_TKZNK = 0x2,  // Track zero not found
    ATA_ERROR_FLAG_ABRT  = 0x4,  // Aborted command
    ATA_ERROR_FLAG_MCR   = 0x8,  // Media change request
    ATA_ERROR_FLAG_IDNF  = 0x10, // ID not found
    ATA_ERROR_FLAG_MC    = 0x20, // Media changed
    ATA_ERROR_FLAG_UNC   = 0x40, // Uncorrectable data error
    ATA_ERROR_FLAG_BBK   = 0x80, // Bad block detected
};

enum ATA_STATUS_FLAG {
    ATA_STATUS_FLAG_ERR = 0x1, // Error occurred
    // ATA_STATUS_FLAG_IDX  = 0x2,  // Index, always zero
    // ATA_STATUS_FLAG_CORR = 0x4,  // Corrected data, always zero
    ATA_STATUS_FLAG_DRQ = 0x8,  // Drive has PIO data to transfer / ready to accept PIO data
    ATA_STATUS_FLAG_SRV = 0x10, // Overlapped mode service request
    ATA_STATUS_FLAG_DF  = 0x20, // Drive fault (does not set ERR)
    ATA_STATUS_FLAG_RDY = 0x40, // Drive is ready (spun up + no errors)
    ATA_STATUS_FLAG_BSY = 0x80, // Drive is preparing to send/receive data
};

enum ATA_CONTROL_FLAG {
    ATA_CONTROL_FLAG_NIEN = 0x2,  // Stop interrupts from the current device
    ATA_CONTROL_FLAG_SRST = 0x4,  // Software reset, set then clear after 5 us
    ATA_CONTROL_FLAG_HOB  = 0x80, // Red high order byte of last LBA48 sent to io
                                  // port
};

enum ATA_ADDRESS_FLAG {
    ATA_ADDRESS_FLAG_DS0 = 0x1,  // Select drive 0
    ATA_ADDRESS_FLAG_DS1 = 0x2,  // Select drive 1
    ATA_ADDRESS_FLAG_HS  = 0x3C, // 1's complement selected head
    ATA_ADDRESS_FLAG_WTG = 0x40, // Low when drive write is in progress
};

static bool ata_identify(ata_t * drive);
static void software_reset(ata_t * drive);

struct _ata {
    uint16_t io_base;
    uint16_t ct_base;
    uint32_t sect_count;
    uint8_t  id;
};

static void ata_callback(registers_t * regs) {
    // TODO add info about call from regs
    KLOG_TRACE("drive callback");
}

ata_t * ata_open(uint8_t id) {
    if (id > 0) {
        KLOG_WARNING("Failed to open drive with id %u, only 0 is supported", id);
        return 0;
    }

    KLOG_INFO("Opening drive %u", id);

    ata_t * drive = kmalloc(sizeof(ata_t));
    if (drive) {
        drive->io_base = ATA_BUS_0_IO_BASE;
        drive->ct_base = ATA_BUS_0_CTL_BASE;
        drive->id      = id;
        if (!ata_identify(drive)) {
            KLOG_ERROR("failed to identify drive %u", id);
            kfree(drive);
            return 0;
        }
        KLOG_INFO("Finished opening ata drive %u with sector count %u", id, drive->sect_count);
    }
    else {
        KLOG_ERROR("Failed to malloc ata_t struct");
    }
    return drive;
}

void ata_close(ata_t * drive) {
    if (!drive) {
        KLOG_WARNING("Tried to free of null pointer");
        return;
    }
    KLOG_DEBUG("Closing drive %u", drive->id);
    kfree(drive);
}

void ata_init() {
    /* Primary Drive */
    KLOG_DEBUG("Registering interrupt handler on IRQ 14");
    register_interrupt_handler(IRQ14, ata_callback);
    KLOG_DEBUG("Initialized driver");
}

size_t ata_size(ata_t * drive) {
    if (!drive) {
        KLOG_WARNING("Tried to get size of null pointer");
        return 0;
    }
    return drive->sect_count * ATA_SECTOR_BYTES;
}

size_t ata_sector_count(ata_t * drive) {
    if (!drive) {
        KLOG_WARNING("Tried to get sector count of null pointer");
        return 0;
    }
    return drive->sect_count;
}

bool ata_status(ata_t * drive) {
    if (!drive) {
        KLOG_WARNING("Tried to get status of null pointer");
        return false;
    }

    uint8_t status = port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);
    if (status & ATA_STATUS_FLAG_ERR) {
        KLOG_DEBUG("Status of drive %u is 0x%02X ERR", drive->id, status);
    }
    if (status & ATA_STATUS_FLAG_DRQ) {
        KLOG_DEBUG("Status of drive %u is 0x%02X DRQ", drive->id, status);
    }
    if (status & ATA_STATUS_FLAG_SRV) {
        KLOG_DEBUG("Status of drive %u is 0x%02X SRV", drive->id, status);
    }
    if (status & ATA_STATUS_FLAG_DF) {
        KLOG_DEBUG("Status of drive %u is 0x%02X DF", drive->id, status);
    }
    if (status & ATA_STATUS_FLAG_RDY) {
        KLOG_DEBUG("Status of drive %u is 0x%02X RDY", drive->id, status);
    }
    if (status & ATA_STATUS_FLAG_BSY) {
        KLOG_DEBUG("Status of drive %u is 0x%02X BSY", drive->id, status);
    }

    if (status & ATA_STATUS_FLAG_ERR) {
        uint8_t error = port_byte_in(drive->io_base + ATA_IO_ERROR);
        if (error & ATA_ERROR_FLAG_AMNF) {
            KLOG_ERROR("AMNF - Address mark not found");
        }
        if (error & ATA_ERROR_FLAG_TKZNK) {
            KLOG_ERROR("TKZNK - Track zero not found");
        }
        if (error & ATA_ERROR_FLAG_ABRT) {
            KLOG_ERROR("ABRT - Aborted command");
        }
        if (error & ATA_ERROR_FLAG_MCR) {
            KLOG_ERROR("MCR - Media change request");
        }
        if (error & ATA_ERROR_FLAG_IDNF) {
            KLOG_ERROR("IDNF - ID not found");
        }
        if (error & ATA_ERROR_FLAG_MC) {
            KLOG_ERROR("MC - Media changed");
        }
        if (error & ATA_ERROR_FLAG_UNC) {
            KLOG_ERROR("UNC - Uncorrectable data error");
        }
        if (error & ATA_ERROR_FLAG_BBK) {
            KLOG_ERROR("BBK - Bad block detected");
        }
        return false;
    }

    return true;
}

size_t ata_sect_read(ata_t * drive, uint8_t * buff, size_t sect_count, uint32_t lba) {
    if (!drive) {
        KLOG_WARNING("Tried to read from a null pointer");
        return 0;
    }
    KLOG_TRACE("Read sector drive=%u buff=%p sector count=%u lba = %u", drive->id, buff, sect_count, lba);
    if (!buff) {
        KLOG_WARNING("Tried to read into a null buffer from drive %u", drive->id);
        return 0;
    }
    if (!sect_count) {
        KLOG_WARNING("Read called with sector count of 0 for drive %u", drive->id);
        return 0;
    }

    // read max 256 sectors at a time
    if (sect_count > 256) {
        KLOG_WARNING("Sector count %u is > 256, truncating at 256 for drive %u read", sect_count, drive->id);
        sect_count = 256;
    }

    if (lba > drive->sect_count) {
        KLOG_WARNING("Read start lba %u is past last sector %u of drive %u", lba, drive->sect_count, drive->id);
        return 0;
    }
    if (lba + sect_count > drive->sect_count) {
        KLOG_WARNING("Read end lba %u past last sector %u, truncating to %u of drive %u", lba + sect_count, drive->sect_count, drive->sect_count - lba, drive->id);
        sect_count = drive->sect_count - lba;
    }
    if (!sect_count) {
        KLOG_WARNING("Read sector count is 0 of drive %u", lba + sect_count, drive->sect_count, drive->sect_count - lba, drive->id);
        return 0;
    }

    software_reset(drive);
    START_TIMEOUT
    size_t retry = 0;
    while (port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS) & (ATA_STATUS_FLAG_DRQ | ATA_STATUS_FLAG_BSY)) {
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            KLOG_ERROR("Max retries %u for ata_sect_read wait for first status of drive %u", MAX_RETRY, drive->id);
            return 0;
        }
    }

    port_byte_out(drive->io_base + ATA_IO_DRIVE_HEAD, (0xE0 | ((lba >> 24) & 0xF)));
    port_byte_out(0x1F1, 0); // delay?
    port_byte_out(drive->io_base + ATA_IO_SECTOR_COUNT, (sect_count >= 256 ? 0 : sect_count));
    port_byte_out(drive->io_base + ATA_IO_LBA_LOW, lba & 0xFF);
    port_byte_out(drive->io_base + ATA_IO_LBA_MID, (lba >> 8) & 0xFF);
    port_byte_out(drive->io_base + ATA_IO_LBA_HIGH, (lba >> 16) & 0xFF);
    port_byte_out(drive->io_base + ATA_IO_COMMAND, 0x20); // read sectors

    for (size_t s = 0; s < sect_count; s++) {
        // Read entire sector
        for (size_t i = 0; i < ATA_SECTOR_WORDS; i++) {
            // Wait for drive to be ready
            retry = 0;
            while (!(port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS) & ATA_STATUS_FLAG_DRQ)) {
                TEST_TIMEOUT
                if (retry++ > MAX_RETRY) {
                    KLOG_ERROR("Max retries %u for ata_sect_read wait to read next sect of drive %u", MAX_RETRY, drive->id);
                    return 0;
                }
            }

            // read drive data
            uint16_t word = port_word_in(drive->io_base + ATA_IO_DATA);

            buff[i * 2]     = word & 0xFF;
            buff[i * 2 + 1] = (word >> 8) & 0xFF;
        }
        buff += ATA_SECTOR_BYTES;
    }

    return sect_count;
}

size_t ata_sect_write(ata_t * drive, const uint8_t * buff, size_t sect_count, uint32_t lba) {
    if (!drive) {
        KLOG_WARNING("Tried to write to a null pointer");
        return 0;
    }
    KLOG_TRACE("Write sector drive=%u buff=%p sector count=%u lba = %u", drive->id, buff, sect_count, lba);
    if (!buff) {
        KLOG_WARNING("Tried to write from a null buffer to drive %u", drive->id);
        return 0;
    }
    if (!sect_count) {
        KLOG_WARNING("Write called with sector count of 0 for drive %u", drive->id);
        return 0;
    }

    // write max 256 sectors at a time
    if (sect_count > 256) {
        KLOG_WARNING("Sector count %u is > 256, truncating at 256 for drive %u write", sect_count, drive->id);
        sect_count = 256;
    }

    if (lba > drive->sect_count) {
        KLOG_WARNING("Write start lba %u is past last sector %u of drive %u", lba, drive->sect_count, drive->id);
        return 0;
    }
    else if (lba + sect_count > drive->sect_count) {
        KLOG_WARNING("Write end lba %u past last sector %u, truncating to %u of drive %u", lba + sect_count, drive->sect_count, drive->sect_count - lba, drive->id);
        sect_count = drive->sect_count - lba;
    }

    software_reset(drive);
    START_TIMEOUT
    uint32_t start = time_ms();
    size_t   retry = 0;
    while (port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS) & (ATA_STATUS_FLAG_DRQ | ATA_STATUS_FLAG_BSY)) {
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            KLOG_ERROR("Max retries %u for ata_sect_write wait for first status of drive %u", MAX_RETRY, drive->id);
            return 0;
        }
    }

    port_byte_out(drive->io_base + ATA_IO_DRIVE_HEAD, (0xE0 | ((lba >> 24) & 0xF)));
    port_byte_out(0x1F1, 0); // delay?
    port_byte_out(drive->io_base + ATA_IO_SECTOR_COUNT, (sect_count >= 256 ? 0 : sect_count));
    port_byte_out(drive->io_base + ATA_IO_LBA_LOW, lba & 0xFF);
    port_byte_out(drive->io_base + ATA_IO_LBA_MID, (lba >> 8) & 0xFF);
    port_byte_out(drive->io_base + ATA_IO_LBA_HIGH, (lba >> 16) & 0xFF);
    port_byte_out(drive->io_base + ATA_IO_COMMAND, 0x30); // write sectors

    size_t o_len = 0;
    for (size_t s = 0; s < sect_count; s++) {
        // Read entire sector
        for (size_t i = 0; i < ATA_SECTOR_WORDS; i++) {
            // Wait for drive to be ready
            retry = 0;
            while (!(port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS) & ATA_STATUS_FLAG_DRQ)) {
                TEST_TIMEOUT
                if (retry++ > MAX_RETRY) {
                    KLOG_ERROR("Max retries %u for ata_sect_write wait to write next sect of drive %u", MAX_RETRY, drive->id);
                    return 0;
                }
            }

            // write drive data
            uint16_t word = buff[i * 2] | (buff[i * 2 + 1] << 8);
            port_word_out(drive->io_base + ATA_IO_DATA, word);
        }
        buff += ATA_SECTOR_BYTES;
    }

    port_byte_out(drive->io_base + ATA_IO_COMMAND, 0xE7); // cache flush
    return sect_count;
}

static bool ata_identify(ata_t * drive) {
    if (!drive) {
        KLOG_WARNING("Tried to identify a null pointer");
        return false;
    }

    START_TIMEOUT
    port_byte_out(drive->io_base + ATA_IO_DRIVE_HEAD, 0xA0);

    port_byte_out(drive->io_base + ATA_IO_LBA_LOW, 0x0);
    port_byte_out(drive->io_base + ATA_IO_LBA_MID, 0x0);
    port_byte_out(drive->io_base + ATA_IO_LBA_HIGH, 0x0);

    port_byte_out(drive->io_base + ATA_IO_COMMAND, 0xEC); // IDENTIFY command
    uint16_t status = port_word_in(drive->io_base + ATA_IO_STATUS);
    KLOG_DEBUG("Status of drive %u is 0x%04X", drive->id, status);

    if (status == 0) {
        KLOG_WARNING("Drive %u does not exist", drive->id);
        return false;
    }

    size_t retry = 0;

    if (status & ATA_STATUS_FLAG_BSY) {
        KLOG_TRACE("Drive %u is busy, polling", drive->id);
        while (status & ATA_STATUS_FLAG_BSY) {
            status = port_byte_in(drive->io_base + ATA_IO_STATUS);
            TEST_TIMEOUT
            if (retry++ > MAX_RETRY) {
                KLOG_ERROR("Max retries for ata_identity wait for first status of drive %u", drive->id);
                return 0;
            }
        }
        KLOG_TRACE("Drive %u is ready", drive->id);
    }

    if (port_byte_in(drive->io_base + ATA_IO_LBA_MID) || port_byte_in(drive->io_base + ATA_IO_LBA_HIGH)) {
        KLOG_WARNING("Drive %u does not support ATA", drive->id);
        return false;
    }

    KLOG_TRACE("Waiting for second status of drive %u", drive->id);
    retry = 0;
    while (!(status & (ATA_STATUS_FLAG_DRQ | ATA_STATUS_FLAG_ERR))) {
        status = port_byte_in(drive->io_base + ATA_IO_STATUS);
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            KLOG_ERROR("Max retries %u for ata_identity wait for second status of drive %u", MAX_RETRY, drive->id);
            return 0;
        }
    }

    if (status & ATA_STATUS_FLAG_ERR) {
        KLOG_WARNING("Drive %u initialized with errors", drive->id);
        return false;
    }

    uint16_t data[ATA_SECTOR_WORDS];
    for (size_t i = 0; i < ATA_SECTOR_WORDS; i++) {
        data[i] = port_word_in(drive->io_base + ATA_IO_DATA);
    }

    bool has_lba = (data[83] & (1 << 10));
    if (has_lba) {
        KLOG_DEBUG("Drive %u has LBA48 Mode", drive->id);
    }

    uint32_t size28 = data[61];
    size28          = size28 << 16;
    size28 |= data[60];

    KLOG_DEBUG("LDA28 of drive %u has %u sectors", drive->id, size28);

    uint64_t size48 = data[100];
    size48          = (size48 << 16) | data[101];
    size48          = (size48 << 16) | data[102];
    size48          = (size48 << 16) | data[103];

    KLOG_DEBUG("LDA48 of drive %u has %u sectors", drive->id, size48);

    drive->sect_count = size28;
    return true;
}

static void software_reset(ata_t * drive) {
    if (!drive) {
        KLOG_WARNING("Tried to reset a null pointer");
        return;
    }
    KLOG_TRACE("Software reset drive %u", drive->id);

    START_TIMEOUT
    port_byte_out(drive->ct_base + ATA_CTL_CONTROL, ATA_CONTROL_FLAG_SRST);
    port_byte_out(drive->ct_base + ATA_CTL_CONTROL, 0);

    // delay 400 ns
    port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);
    port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);
    port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);
    port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);

    uint8_t status = port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);
    // while ((status & (ATA_STATUS_FLAG_RDY | ATA_STATUS_FLAG_BSY)) !=
    // ATA_STATUS_FLAG_RDY) {
    size_t retry = 0;
    while ((status & 0xc0) != 0x40) {
        status = port_byte_in(drive->ct_base + ATA_CTL_ALT_STATUS);
        TEST_TIMEOUT_VOID
        if (retry++ > MAX_RETRY) {
            KLOG_ERROR("Max retries for software_reset wait for drive %u", drive->id);
            return;
        }
    }
}
