#define KLOG_SERVICE "DRIVERS/RAMDISK"

#include "drivers/ramdisk.h"

#include "kernel.h"
#include "kernel/logs.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/string.h"

struct _ramdisk {
    int    id;
    size_t size;
    void * data;
};

static ramdisk_t __drives[RAMDISK_MAX];
static int       __drive_count = 0;

int ramdisk_create(size_t size) {
    if (__drive_count == RAMDISK_MAX) {
        KLOG_ERROR("Tried to create drive after reaching max count %u", RAMDISK_MAX);
        return -1;
    }

    void * data = kmalloc(size);
    if (!data) {
        KLOG_ERROR("Failed to allocate drive of size %u", size);
        return -1;
    }

    __drives[__drive_count].id   = __drive_count;
    __drives[__drive_count].size = size;
    __drives[__drive_count].data = data;

    KLOG_DEBUG("Created new drive %u with size %u", __drive_count, size);

    return __drive_count++;
}

ramdisk_t * ramdisk_open(int id) {
    if (id < 0 || id >= __drive_count) {
        KLOG_ERROR("Failed to open invalid drive id %d", id);
        return 0;
    }
    return &__drives[id];
}

void ramdisk_close(ramdisk_t * drive) {
    if (!drive) {
        KLOG_ERROR("Tried to close null drive");
        return;
    }
    KLOG_WARNING("Close does not yet free the drive for future use");
}

size_t ramdisk_size(ramdisk_t * drive) {
    if (!drive) {
        KLOG_ERROR("Tried to get size of null drive");
        return 0;
    }
    return drive->size;
}

size_t ramdisk_read(ramdisk_t * drive, uint8_t * buff, size_t count, size_t pos) {
    if (!drive) {
        KLOG_ERROR("Tried to get size of null pointer");
        return 0;
    }
    if (!buff) {
        KLOG_ERROR("Tried to read into a null buffer from drive %u", drive->id);
        return 0;
    }
    if (!count) {
        KLOG_WARNING("Read called with sector count of 0 for drive %u", drive->id);
        return 0;
    }

    if (pos > drive->size) {
        KLOG_WARNING("Read start of %u is after end %u of drive %u", pos, drive->size, drive->id);
        return 0;
    }
    if (drive->size - pos < count) {
        KLOG_WARNING("Read size %u will end past the device end, truncating to %u for drive %u", count, drive->size - pos, drive->id);
        count = drive->size - pos;
    }
    if (count < 0) {
        KLOG_WARNING("Read size is 0 for drive %u", drive->id);
        return 0;
    }

    if (!kmemcpy(buff, drive->data + pos, count)) {
        KLOG_ERROR("Failed to copy %u bytes in memory from %p to %p", count, drive->data + pos, buff);
        return 0;
    }
    return count;
}

size_t ramdisk_write(ramdisk_t * drive, uint8_t * buff, size_t count, size_t pos) {
    if (drive->size - pos < count) {
        count = drive->size - pos;
    }

    if (!kmemcpy(buff, drive->data + pos, count)) {
        KLOG_ERROR("Failed to copy %u bytes in memory from %p to %p", count, drive->data + pos, buff);
        return 0;
    }
    return count;
}
