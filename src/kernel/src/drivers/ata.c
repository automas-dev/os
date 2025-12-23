#include "drivers/ata.h"

#include "cpu/isr.h"
#include "cpu/ports.h"
#include "debug.h"
#include "drivers/rtc.h"
#include "kernel.h"
#include "kernel/time.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"

// https://wiki.osdev.org/ATA_PIO_Mode

#if SAFETY > 1
#include "libc/stdio.h"
#define TEST_PTR(REF)          \
    if (!(REF)) {              \
        printf(                \
            "[ERROR] "__FILE__ \
            ":%u Null ptr\n",  \
            __LINE__);         \
        return;                \
    }
#define TEST_PTR_RET(REF)      \
    if (!(REF)) {              \
        printf(                \
            "[ERROR] "__FILE__ \
            ":%u Null ptr\n",  \
            __LINE__);         \
        return 0;              \
    }
#else
#define TEST_PTR(REF)
#define TEST_PTR_RET(REF)
#endif

#define MAX_RETRY     5000
#define TIMEOUT_MS    1000
#define START_TIMEOUT uint32_t __timeout = time_ms() + TIMEOUT_MS;
#define TEST_TIMEOUT                                          \
    if (time_ms() > __timeout) {                              \
        puts("TIMEOUT\n");                                    \
        return 0;                                             \
    }                                                         \
    else if (debug) {                                         \
        printf("no timeout %u < %u\n", time_ms(), __timeout); \
    }
#define TEST_TIMEOUT_VOID                                     \
    if (time_ms() > __timeout) {                              \
        puts("TIMEOUT\n");                                    \
        return;                                               \
    }                                                         \
    else if (debug) {                                         \
        printf("no timeout %u < %u\n", time_ms(), __timeout); \
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

static bool ata_identify(ata_t * disk);
static void software_reset(ata_t * disk);

struct _ata {
    uint16_t io_base;
    uint16_t ct_base;
    uint32_t sect_count;
};

static void ata_callback(registers_t * regs) {
    if (debug) {
        puts("disk callback\n");
    }
}

ata_t * ata_open(uint8_t id) {
    if (id > 0) {
        return 0;
    }

    ata_t * disk = kmalloc(sizeof(ata_t));
    if (disk) {
        disk->io_base = ATA_BUS_0_IO_BASE;
        disk->ct_base = ATA_BUS_0_CTL_BASE;
        if (!ata_identify(disk)) {
            puts("ERROR: failed to identify disk\n");
            kfree(disk);
            return 0;
        }
    }
    return disk;
}

void ata_close(ata_t * disk) {
    kfree(disk);
}

void ata_init() {
    /* Primary Drive */
    register_interrupt_handler(IRQ14, ata_callback);
}

size_t ata_size(ata_t * disk) {
    if (!disk) {
        return 0;
    }
    return disk->sect_count * ATA_SECTOR_BYTES;
}

size_t ata_sector_count(ata_t * disk) {
    if (!disk) {
        return 0;
    }
    return disk->sect_count;
}

bool ata_status(ata_t * disk) {
    if (!disk) {
        return false;
    }

    uint8_t status = port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);
    if (debug) {
        printf("Status is %02X\n", status);
        if (status & ATA_STATUS_FLAG_ERR) {
            puts("ERR ");
        }
        if (status & ATA_STATUS_FLAG_DRQ) {
            puts("DRQ ");
        }
        if (status & ATA_STATUS_FLAG_SRV) {
            puts("SRV ");
        }
        if (status & ATA_STATUS_FLAG_DF) {
            puts("DF ");
        }
        if (status & ATA_STATUS_FLAG_RDY) {
            puts("RDY ");
        }
        if (status & ATA_STATUS_FLAG_BSY) {
            puts("BSY ");
        }
        putc('\n');
    }

    if (status & ATA_STATUS_FLAG_ERR) {
        if (debug) {
            uint8_t error = port_byte_in(disk->io_base + ATA_IO_ERROR);
            if (error & ATA_ERROR_FLAG_AMNF) {
                puts("ERROR: AMNF - Address mark not found\n");
            }
            if (error & ATA_ERROR_FLAG_TKZNK) {
                puts("ERROR: TKZNK - Track zero not found\n");
            }
            if (error & ATA_ERROR_FLAG_ABRT) {
                puts("ERROR: ABRT - Aborted command\n");
            }
            if (error & ATA_ERROR_FLAG_MCR) {
                puts("ERROR: MCR - Media change request\n");
            }
            if (error & ATA_ERROR_FLAG_IDNF) {
                puts("ERROR: IDNF - ID not found\n");
            }
            if (error & ATA_ERROR_FLAG_MC) {
                puts("ERROR: MC - Media changed\n");
            }
            if (error & ATA_ERROR_FLAG_UNC) {
                puts("ERROR: UNC - Uncorrectable data error\n");
            }
            if (error & ATA_ERROR_FLAG_BBK) {
                puts("ERROR: BBK - Bad block detected\n");
            }
        }
        return true;
    }

    return false;
}

size_t ata_sect_read(ata_t * disk, uint8_t * buff, size_t sect_count, uint32_t lba) {
    if (!disk || !buff || !sect_count) {
        return 0;
    }

    // read max 256 sectors at a time
    if (sect_count > 256) {
        sect_count = 256;
    }

    if (lba > disk->sect_count) {
        return 0;
    }
    else if (lba + sect_count > disk->sect_count) {
        sect_count = disk->sect_count - lba;
    }

    software_reset(disk);
    START_TIMEOUT
    size_t retry = 0;
    while (port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS) & (ATA_STATUS_FLAG_DRQ | ATA_STATUS_FLAG_BSY)) {
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            puts("[ERROR] max retries for ata_sect_read wait for first status\n");
            return 0;
        }
    }

    port_byte_out(disk->io_base + ATA_IO_DRIVE_HEAD, (0xE0 | ((lba >> 24) & 0xF)));
    port_byte_out(0x1F1, 0); // delay?
    port_byte_out(disk->io_base + ATA_IO_SECTOR_COUNT, (sect_count >= 256 ? 0 : sect_count));
    port_byte_out(disk->io_base + ATA_IO_LBA_LOW, lba & 0xFF);
    port_byte_out(disk->io_base + ATA_IO_LBA_MID, (lba >> 8) & 0xFF);
    port_byte_out(disk->io_base + ATA_IO_LBA_HIGH, (lba >> 16) & 0xFF);
    port_byte_out(disk->io_base + ATA_IO_COMMAND, 0x20); // read sectors

    for (size_t s = 0; s < sect_count; s++) {
        // Read entire sector
        for (size_t i = 0; i < ATA_SECTOR_WORDS; i++) {
            // Wait for drive to be ready
            retry = 0;
            while (!(port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS) & ATA_STATUS_FLAG_DRQ)) {
                TEST_TIMEOUT
                if (retry++ > MAX_RETRY) {
                    puts("[ERROR] max retries for ata_sect_read wait to read next sect\n");
                    return 0;
                }
            }

            // read drive data
            uint16_t word = port_word_in(disk->io_base + ATA_IO_DATA);

            buff[i * 2]     = word & 0xFF;
            buff[i * 2 + 1] = (word >> 8) & 0xFF;
        }
        buff += ATA_SECTOR_BYTES;
    }

    return sect_count;
}

size_t ata_sect_write(ata_t * disk, uint8_t * buff, size_t sect_count, uint32_t lba) {
    if (!disk || !buff || !sect_count) {
        return 0;
    }

    // write max 256 sectors at a time
    if (sect_count > 256) {
        sect_count = 256;
    }

    if (lba > disk->sect_count) {
        return 0;
    }
    else if (lba + sect_count > disk->sect_count) {
        sect_count = disk->sect_count - lba;
    }

    software_reset(disk);
    START_TIMEOUT
    uint32_t start = time_ms();
    size_t   retry = 0;
    while (port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS) & (ATA_STATUS_FLAG_DRQ | ATA_STATUS_FLAG_BSY)) {
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            puts("[ERROR] max retries for ata_sect_write wait for first status\n");
            return 0;
        }
    }

    port_byte_out(disk->io_base + ATA_IO_DRIVE_HEAD, (0xE0 | ((lba >> 24) & 0xF)));
    port_byte_out(0x1F1, 0); // delay?
    port_byte_out(disk->io_base + ATA_IO_SECTOR_COUNT, (sect_count >= 256 ? 0 : sect_count));
    port_byte_out(disk->io_base + ATA_IO_LBA_LOW, lba & 0xFF);
    port_byte_out(disk->io_base + ATA_IO_LBA_MID, (lba >> 8) & 0xFF);
    port_byte_out(disk->io_base + ATA_IO_LBA_HIGH, (lba >> 16) & 0xFF);
    port_byte_out(disk->io_base + ATA_IO_COMMAND, 0x30); // write sectors

    size_t o_len = 0;
    for (size_t s = 0; s < sect_count; s++) {
        // Read entire sector
        for (size_t i = 0; i < ATA_SECTOR_WORDS; i++) {
            // Wait for drive to be ready
            retry = 0;
            while (!(port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS) & ATA_STATUS_FLAG_DRQ)) {
                TEST_TIMEOUT
                if (retry++ > MAX_RETRY) {
                    puts("[ERROR] max retries for ata_sect_write wait to write next sect\n");
                    return 0;
                }
            }

            // write drive data
            uint16_t word = buff[i * 2] | (buff[i * 2 + 1] << 8);
            port_word_out(disk->io_base + ATA_IO_DATA, word);
        }
        buff += ATA_SECTOR_BYTES;
    }

    port_byte_out(disk->io_base + ATA_IO_COMMAND, 0xE7); // cache flush
    return sect_count;
}

static bool ata_identify(ata_t * disk) {
    if (!disk) {
        return false;
    }

    START_TIMEOUT
    port_byte_out(disk->io_base + ATA_IO_DRIVE_HEAD, 0xA0);

    port_byte_out(disk->io_base + ATA_IO_LBA_LOW, 0x0);
    port_byte_out(disk->io_base + ATA_IO_LBA_MID, 0x0);
    port_byte_out(disk->io_base + ATA_IO_LBA_HIGH, 0x0);

    port_byte_out(disk->io_base + ATA_IO_COMMAND, 0xEC); // IDENTIFY command
    uint16_t status = port_word_in(disk->io_base + ATA_IO_STATUS);

    if (status == 0) {
        puts("Drive does not exist\n");
        return false;
    }

    if (debug) {
        puts("Polling");
    }
    size_t retry = 0;
    while (status & ATA_STATUS_FLAG_BSY) {
        if (debug) {
            putc('.');
        }
        status = port_byte_in(disk->io_base + ATA_IO_STATUS);
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            puts("[ERROR] max retries for ata_identity wait for first status\n");
            return 0;
        }
    }
    if (debug) {
        putc('\n');
    }

    if (port_byte_in(disk->io_base + ATA_IO_LBA_MID) || port_byte_in(disk->io_base + ATA_IO_LBA_HIGH)) {
        puts("Disk does not support ATA\n");
        return false;
    }
    if (debug) {
        puts("Drive is ATA\n");
    }

    if (debug) {
        puts("Polling");
    }
    retry = 0;
    while (!(status & (ATA_STATUS_FLAG_DRQ | ATA_STATUS_FLAG_ERR))) {
        if (debug) {
            putc('.');
        }
        status = port_byte_in(disk->io_base + ATA_IO_STATUS);
        TEST_TIMEOUT
        if (retry++ > MAX_RETRY) {
            puts("[ERROR] max retries for ata_identity wait for second status\n");
            return 0;
        }
    }
    if (debug) {
        putc('\n');
    }

    if (status & ATA_STATUS_FLAG_ERR) {
        puts("Disk initialized with errors\n");
        return false;
    }

    if (status & ATA_STATUS_FLAG_DRQ) {
        if (debug) {
            puts("Disk is ready\n");
        }
    }

    uint16_t data[ATA_SECTOR_WORDS];
    for (size_t i = 0; i < ATA_SECTOR_WORDS; i++) {
        data[i] = port_word_in(disk->io_base + ATA_IO_DATA);
    }

    if (debug) {
        printf("Data is:\n");
        size_t step = 8;
        for (size_t i = 0; i < (ATA_SECTOR_WORDS / step); i++) {
            printf("%4u", i * step);
            for (size_t s = 0; s < step; s++) {
                printf(" %04X", data[(i * step) + s]);
            }
            putc('\n');
        }
    }

    bool has_lba = (data[83] & (1 << 10));
    if (has_lba) {
        if (debug) {
            printf("Drive has LBA48 Mode\n");
        }
    }

    uint32_t size28 = data[61];
    size28          = size28 << 16;
    size28 |= data[60];

    if (debug) {
        printf("LDA28 has %u sectors\n", size28);
    }

    uint64_t size48 = data[100];
    size48          = (size48 << 16) | data[101];
    size48          = (size48 << 16) | data[102];
    size48          = (size48 << 16) | data[103];

    if (debug) {
        printf("LDA48 has %u sectors\n", size48);
    }

    disk->sect_count = size28;
    return true;
}

static void software_reset(ata_t * disk) {
    if (!disk) {
        return;
    }

    START_TIMEOUT
    port_byte_out(disk->ct_base + ATA_CTL_CONTROL, ATA_CONTROL_FLAG_SRST);
    port_byte_out(disk->ct_base + ATA_CTL_CONTROL, 0);

    // delay 400 ns
    port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);
    port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);
    port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);
    port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);

    uint8_t status = port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);
    // while ((status & (ATA_STATUS_FLAG_RDY | ATA_STATUS_FLAG_BSY)) !=
    // ATA_STATUS_FLAG_RDY) {
    size_t retry = 0;
    while ((status & 0xc0) != 0x40) {
        status = port_byte_in(disk->ct_base + ATA_CTL_ALT_STATUS);
        TEST_TIMEOUT_VOID
        if (retry++ > MAX_RETRY) {
            puts("[ERROR] max retries for software_reset wait for drive\n");
            return;
        }
    }
}
