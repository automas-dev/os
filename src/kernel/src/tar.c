#include "drivers/tar.h"

#include <stdint.h>

#include "kernel.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/stdio.h"
#include "libc/string.h"

typedef struct {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    // NOTE all field are ascii numbers, size is base 8
    char size[12];
    char mtime[12];
    char checksum[8];
    char type[1];
    // There are 355 empty bytes to make header 512 long
    // uint8_t reserved[355];
} __attribute__((packed)) raw_header_t;

typedef struct {
    char   filename[101];
    size_t size;
    size_t index;
    size_t disk_pos;

    raw_header_t header;
} tar_file_t;

struct tar_fs {
    disk_t *     disk;
    size_t       file_count;
    tar_file_t * files;
} __attribute__((packed));

struct tar_fs_file {
    tar_fs_t *   tar;
    tar_file_t * file;
    int          pos;
    size_t       size;
};

static size_t       parse_octal(const char * str);
static size_t       count_files(tar_fs_t * tar);
static bool         load_headers(tar_fs_t * tar);
static tar_file_t * find_filename(tar_fs_t * tar, const char * filename);

tar_fs_t * tar_open(disk_t * disk) {
    if (!disk) {
        return 0;
    }

    tar_fs_t * tar = kmalloc(sizeof(tar_fs_t));
    if (tar) {
        tar->disk       = disk;
        tar->file_count = count_files(tar);
        tar->files      = kmalloc(sizeof(tar_file_t) * tar->file_count);
        load_headers(tar);
    }
    return tar;
}

void tar_close(tar_fs_t * tar) {
    kfree(tar);
}

size_t tar_file_count(tar_fs_t * tar) {
    if (!tar) {
        return 0;
    }

    return tar->file_count;
}

const char * tar_file_name(tar_fs_t * tar, size_t i) {
    if (!tar || i > tar->file_count) {
        return 0;
    }

    return tar->files[i].filename;
}

size_t tar_file_size(tar_fs_t * tar, size_t i) {
    if (!tar || i > tar->file_count) {
        return 0;
    }

    return tar->files[i].size;
}

tar_stat_t * tar_stat_file_i(tar_fs_t * tar, size_t i, tar_stat_t * stat) {
    if (!tar || !stat || i > tar->file_count) {
        return 0;
    }

    tar_file_t * file = &tar->files[i];
    kmemcpy(stat->filename, file->filename, 101);
    stat->size  = file->size;
    stat->mode  = parse_octal(file->header.mode);
    stat->uid   = parse_octal(file->header.uid);
    stat->gid   = parse_octal(file->header.gid);
    stat->mtime = parse_octal(file->header.mtime);
    stat->type  = parse_octal(file->header.type);

    return stat;
}

tar_stat_t * tar_stat_file(tar_fs_t * tar, const char * filename, tar_stat_t * stat) {
    if (!tar || !filename || !stat) {
        return 0;
    }

    tar_file_t * file = find_filename(tar, filename);
    if (!file) {
        return 0;
    }

    kmemcpy(stat->filename, file->filename, 101);
    stat->size  = file->size;
    stat->mode  = parse_octal(file->header.mode);
    stat->uid   = parse_octal(file->header.uid);
    stat->gid   = parse_octal(file->header.gid);
    stat->mtime = parse_octal(file->header.mtime);
    stat->type  = parse_octal(file->header.type);

    return stat;
}

tar_fs_file_t * tar_file_open(tar_fs_t * tar, const char * filename) {
    if (!tar || !filename) {
        return 0;
    }

    tar_fs_file_t * file = kmalloc(sizeof(tar_fs_file_t));
    if (file) {
        file->tar  = tar;
        file->file = find_filename(tar, filename);
        file->pos  = 0;
        file->size = file->file->size;

        if (!file->file) {
            kfree(file);
            return 0;
        }
    }
    return file;
}

void tar_file_close(tar_fs_file_t * file) {
    kfree(file);
}

bool tar_file_seek(tar_fs_file_t * file, int offset, enum TAR_SEEK_ORIGIN origin) {
    if (!file) {
        return false;
    }

    switch (origin) {
        default:
        case TAR_SEEK_ORIGIN_START: {
            if (offset < 0) {
                file->pos = 0;
            }
            else if (offset > file->size) {
                file->pos = file->size;
            }
            else {
                file->pos = offset;
            }
        } break;
        case TAR_SEEK_ORIGIN_END: {
            if (offset > 0) {
                file->pos = file->size;
            }
            else if (offset < file->size) {
                file->pos = 0;
            }
            else {
                file->pos = file->size - offset;
            }
        } break;
        case TAR_SEEK_ORIGIN_CURRENT: {
            if (offset > file->size - file->pos) {
                file->pos = file->size;
            }
            else if (offset < -file->pos) {
                file->pos = 0;
            }
            else {
                file->pos = file->pos + offset;
            }
        } break;
    }
    return true;
}

int tar_file_tell(tar_fs_file_t * file) {
    if (!file) {
        return -1;
    }
    return file->pos;
}

size_t tar_file_read(tar_fs_file_t * file, char * buff, size_t count) {
    if (!file || !buff) {
        return 0;
    }

    if (file->pos + count > file->size) {
        count = file->size - file->pos;
    }

    return disk_read(file->tar->disk, buff, count, file->file->disk_pos + file->pos);
}

static size_t parse_octal(const char * str) {
    if (!str) {
        return 0;
    }

    size_t size  = 0;
    size_t count = 1;

    for (size_t j = kstrlen(str); j > 0; j--, count *= 8) {
        size += ((str[j - 1] - '0') * count);
    }

    return size;
}

static size_t count_files(tar_fs_t * tar) {
    if (!tar) {
        return 0;
    }

    raw_header_t header;

    size_t disk_pos = 0;
    size_t count    = 0;
    for (;;) {
        size_t n_read = disk_read(tar->disk, (uint8_t *)&header, sizeof(raw_header_t), disk_pos);

        if (!n_read) {
            break;
        }

        if (n_read != sizeof(raw_header_t)) {
            if (debug) {
                printf("Read only got %u bytes\n", n_read);
            }
            PANIC("Failed to read full tar header from disk");
        }

        if (header.filename[0] == 0) {
            break;
        }

        size_t file_size = parse_octal(header.size);
        count++;

        // add 512 for header size
        disk_pos += 512 + file_size;
        if (disk_pos % 512) {
            disk_pos += (512 - (disk_pos % 512));
        }
    }

    return count;
}

static bool load_headers(tar_fs_t * tar) {
    if (!tar) {
        return false;
    }

    size_t disk_pos = 0;

    for (size_t i = 0; i < tar->file_count; i++) {
        tar_file_t * file = &tar->files[i];

        size_t n_read = disk_read(tar->disk, (uint8_t *)&file->header, sizeof(raw_header_t), disk_pos);

        if (!n_read) {
            break;
        }

        if (n_read != sizeof(raw_header_t)) {
            if (debug) {
                printf("Read only got %u bytes\n", n_read);
            }
            PANIC("Failed to read full tar header from disk");
        }

        size_t file_size = file->size = parse_octal(file->header.size);

        size_t name_len = knstrlen(file->header.filename, 100);
        kmemcpy(file->filename, file->header.filename, name_len);
        file->filename[name_len] = 0;

        file->index    = i;
        file->disk_pos = disk_pos + 512;

        // add 512 for header size
        disk_pos += 512 + file_size;
        if (disk_pos % 512) {
            disk_pos += (512 - (disk_pos % 512));
        }
    }

    return true;
}

static tar_file_t * find_filename(tar_fs_t * tar, const char * filename) {
    if (!tar || !filename || !kstrlen(filename)) {
        return 0;
    }

    if (filename[0] == '/') {
        filename++;
    }

    for (size_t i = 0; i < tar->file_count; i++) {
        size_t size          = knstrlen(filename, 100);
        size_t filename_size = knstrlen(tar->files[i].filename, 100);
        if (filename_size != size) {
            continue;
        }
        if (kmemcmp(filename, tar->files[i].filename, size) == 0) {
            return &tar->files[i];
        }
    }
    return 0;
}
