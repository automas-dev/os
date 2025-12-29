#include "commands.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cpu/ports.h"
#include "debug.h"
#include "drivers/disk.h"
#include "drivers/ram.h"
#include "drivers/rtc.h"
#include "drivers/tar.h"
#include "drivers/vga.h"
#include "ebus.h"
#include "exec.h"
#include "kernel.h"
#include "kernel/time.h"
#include "libc/datastruct/array.h"
#include "libc/memory.h"
#include "libc/proc.h"
#include "libc/signal.h"
#include "libc/stdio.h"
#include "libc/string.h"
#include "paging.h"
#include "process.h"

bool debug = false;

static int clear_cmd(size_t argc, char ** argv) {
    vga_clear();
    return 0;
}

static int echo_cmd(size_t argc, char ** argv) {
    bool next_line = true;
    if (argc > 1 && kmemcmp(argv[1], "-n", 2) == 0) {
        next_line = false;
    }

    size_t i = 1;
    if (!next_line) {
        i++;
    }
    for (; i < argc; i++) {
        puts(argv[i]);
        if (i < argc) {
            putc(' ');
        }
    }

    if (next_line) {
        putc('\n');
    }

    return 0;
}

static int debug_cmd(size_t argc, char ** argv) {
    debug = !debug;
    if (debug) {
        puts("Enable debug\n");
    }
    else {
        puts("Disabling debug\n");
    }
    return 0;
}

static int atoi_cmd(size_t argc, char ** argv) {
    if (argc != 2) {
        printf("Usage: %s <number>\n", argv[0]);
        return 1;
    }

    int i = katoi(argv[1]);
    printf("%d\n", i);
    return 0;
}

static uint8_t hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    }
    else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    }
    else {
        return 0;
    }
}

static uint16_t parse_byte(const char * str) {
    size_t len = kstrlen(str);
    if (len < 1 || len > 4) {
        return 1;
    }

    uint16_t res = 0;
    while (*str) {
        res = (res << 4) | hex_digit(*str++);
    }
    return res;
}

static int port_out_cmd(size_t argc, char ** argv) {
    if (argc != 3) {
        printf("Usage: %s <port> <byte>\n", argv[0]);
        return 1;
    }

    if (kstrlen(argv[1]) > 4) {
        puts("port has max 4 hex digits\n");
        return 1;
    }
    uint16_t port = parse_byte(argv[1]);

    if (kstrlen(argv[2]) > 2) {
        puts("byte has max 2 hex digits\n");
        return 1;
    }
    uint8_t byte = parse_byte(argv[2]);

    port_byte_out(port, byte);
    return 0;
}

static int port_in_cmd(size_t argc, char ** argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    if (kstrlen(argv[1]) > 4) {
        puts("port has max 4 hex digits\n");
        return 1;
    }
    uint16_t port = parse_byte(argv[1]);

    uint8_t byte = port_byte_in(port);
    printf("0x%04X = 0x%02X\n", port, byte);

    return 0;
}

static int time_cmd(size_t argc, char ** argv) {
    uint32_t ticks = time_ticks();
    uint32_t s     = time_s();
    uint32_t ms    = time_ms();
    uint32_t ns    = time_ns();
    printf("System ticks: %u ~= %u ns ~= %u ms ~= %u s\n", ticks, ns, ms, s);
    printf("RTC time: %u us = %u ms = %u s\n", time_us(), time_ms(), time_s());
    return 0;
}

int sleep_handler;

static void sleep_cb(const ebus_event_t * event) {
    printf("Timer ended at %u\n", event->timer.time);
    ebus_unregister_handler(get_kernel_ebus(), sleep_handler);
    sleep_handler = 0;
}

static int sleep_cmd(size_t argc, char ** argv) {
    if (sleep_handler) {
        printf("Please wait for sleep to finish\n");
        return 1;
    }

    ebus_handler_t handler = {0};
    handler.callback_fn    = sleep_cb;
    handler.event_id       = EBUS_EVENT_TIMER;

    sleep_handler = ebus_register_handler(get_kernel_ebus(), &handler);
    time_start_timer_ms(1000);
}

static int ret_cmd(size_t argc, char ** argv) {
    // printf("Last command exit code was %u\n", term_last_ret);
    return 0;
}

// static int format_cmd(size_t argc, char ** argv) {
//     if (fs) {
//         puts("Unmount disk before format\n");
//         return 1;
//     }

//     if (!kernel_get_disk()) {
//         disk = disk_open(0, DISK_DRIVER_ATA);
//         if (!kernel_get_disk()) {
//             puts("Failed to open disk\n");
//             return 1;
//         }
//     }

//     puts("Formatting disk, this may take some time\n");
//     // fs_format(kernel_get_disk());
//     puts("Done!\n");

//     return 0;
// }

// static int mount_cmd(size_t argc, char ** argv) {
//     if (kernel_get_tar()) {
//         puts("Filesystem already mounted\n");
//         return 0;
//     }

//     if (!kernel_get_disk()) {
//         disk = disk_open(0, DISK_DRIVER_ATA);
//         if (!kernel_get_disk()) {
//             puts("Failed to open disk\n");
//             return 1;
//         }
//     }

//     tar = tar_open(kernel_get_disk());
//     if (!kernel_get_tar()) {
//         puts("Failed to mount filesystem\n");
//         return 1;
//     }

//     return 0;
// }

// static int unmount_cmd(size_t argc, char ** argv) {
//     if (kernel_get_tar()) {
//         tar_close(kernel_get_tar());
//         tar = 0;
//     }
//     if (kernel_get_disk()) {
//         disk_close(kernel_get_disk());
//         disk = 0;
//     }
//     return 0;
// }

static void print_64(uint64_t v) {
    uint32_t u = v >> 32;
    uint32_t l = v & 0xffffffff;
    printf("0x%08X%08X", u, l);
}

// static void print_upper(uint64_t start, uint64_t end, uint64_t size, enum RAM_TYPE type) {
//     puts("| ");
//     print_64(start);
//     puts(" | ");
//     print_64(end);
//     puts(" | ");
//     print_64(size);
//     puts(" | ");
//     switch (type) {
//         case 1:
//             puts("Usable RAM");
//             break;
//         case 2:
//             puts("Reserved");
//             break;
//         case 3:
//             puts("ACPI Reclaimable");
//             break;
//         case 4:
//             puts("ACPI NVS");
//             break;
//         case 5:
//             puts("BAD MEMORY");
//             break;
//     }
//     putc('\n');
// }

// static int mem_cmd(size_t argc, char ** argv) {
//     uint16_t count = ram_upper_count();

//     printf("Total of %u blocks\n", count);

//     puts("| Start              | End                | Size               | Type\n");
//     for (size_t i = 0; i < count; i++) {
//         print_upper(ram_upper_start(i), ram_upper_end(i), ram_upper_size(i), ram_upper_type(i));
//     }
//     return 0;
// }

static void ls_print_file_size(size_t size) {
    size_t order = 0;
    while (size > 1024) {
        size /= 1024;
        order++;
    }
    char order_c = ' ';
    switch (order) {
        case 1:
            order_c = 'K';
            break;
        case 2:
            order_c = 'M';
            break;
        case 3:
            order_c = 'G';
            break;
        default:
            break;
    }
    printf("%4u%c", size, order_c);
}

static void ls_print_file(tar_stat_t * stat) {
    putc(stat->type == TAR_TYPE_DIR ? 'd' : '-');

    putc(stat->mode & TAR_PEM_USER_READ ? 'r' : '-');
    putc(stat->mode & TAR_PEM_USER_WRITE ? 'w' : '-');
    putc(stat->mode & TAR_PEM_USER_EXECUTE ? 'x' : '-');

    putc(stat->mode & TAR_PEM_GROUP_READ ? 'r' : '-');
    putc(stat->mode & TAR_PEM_GROUP_WRITE ? 'w' : '-');
    putc(stat->mode & TAR_PEM_GROUP_EXECUTE ? 'x' : '-');

    putc(stat->mode & TAR_PEM_OTHER_READ ? 'r' : '-');
    putc(stat->mode & TAR_PEM_OTHER_WRITE ? 'w' : '-');
    putc(stat->mode & TAR_PEM_OTHER_EXECUTE ? 'x' : '-');

    printf(" %4u %4u ", stat->uid, stat->gid);
    ls_print_file_size(stat->size);

    printf(" %4u ", stat->mtime);
    puts(stat->filename);
    putc('\n');
}

static int ls_cmd(size_t argc, char ** argv) {
    if (!kernel_get_tar()) {
        puts("Filesystem not mounted\n");
        return 1;
    }

    size_t file_count = tar_file_count(kernel_get_tar());
    printf("Found %u files\n", file_count);

    tar_stat_t stat;
    for (size_t i = 0; i < file_count; i++) {
        if (!tar_stat_file_i(kernel_get_tar(), i, &stat)) {
            return 1;
        }
        ls_print_file(&stat);
    }

    putc('\n');
    return 0;
}

static int stat_cmd(size_t argc, char ** argv) {
    if (!kernel_get_tar()) {
        puts("Filesystem not mounted\n");
        return 1;
    }

    tar_stat_t stat;
    if (!tar_stat_file_i(kernel_get_tar(), 0, &stat)) {
        puts("Failed to stat file\n");
        return 1;
    }

    printf("File %s\n", stat.filename);
    printf("Size %u\n", stat.size);
    printf("%04x (%u : %u)\n", stat.mode, stat.uid, stat.gid);
    printf("mtime %04x\n", stat.mtime);
    printf("Type %02x\n", stat.type);

    return 0;
}

static int fs_read_cmd(size_t argc, char ** argv) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char * filename = argv[1];

    if (!kernel_get_tar()) {
        puts("Filesystem not mounted\n");
        return 1;
    }

    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        puts("Failed to find file\n");
        return 1;
    }

    ls_print_file(&stat);
    uint8_t * buff = kmalloc(stat.size);
    if (!buff) {
        return 1;
    }

    tar_fs_file_t * file = tar_file_open(kernel_get_tar(), filename);
    if (!file) {
        kfree(buff);
        return 1;
    }

    if (!tar_file_read(file, buff, stat.size)) {
        tar_file_close(file);
        kfree(buff);
        return 1;
    }
    print_hexblock(buff, stat.size, 0);

    if (!buff) {
        return 0;
    }

    tar_file_close(file);
    kfree(buff);

    return 0;
}

static int fs_cat_cmd(size_t argc, char ** argv) {
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    char * filename = argv[1];

    if (!kernel_get_tar()) {
        puts("Filesystem not mounted\n");
        return 1;
    }

    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        puts("Failed to find file\n");
        return 1;
    }

    uint8_t * buff = kmalloc(stat.size);
    if (!buff) {
        return 1;
    }

    tar_fs_file_t * file = tar_file_open(kernel_get_tar(), filename);
    if (!file) {
        kfree(buff);
        return 1;
    }

    if (!tar_file_read(file, buff, stat.size)) {
        tar_file_close(file);
        kfree(buff);
        return 1;
    }
    for (size_t i = 0; i < stat.size; i++) {
        putc(buff[i]);
    }

    if (!buff) {
        return 0;
    }

    tar_file_close(file);
    kfree(buff);

    return 0;
}

// static int status_cmd(size_t argc, char ** argv) {
//     // ata_status();
//     return 0;
// }

static int disk_read_cmd(size_t argc, char ** argv) {
    if (argc != 3) {
        printf("Usage: %s <pos> <count>\n", argv[0]);
        return 1;
    }

    size_t pos   = katoi(argv[1]);
    size_t count = katoi(argv[2]);

    // if (!kernel_get_disk()) {
    //     disk = disk_open(0, DISK_DRIVER_ATA);
    //     if (!kernel_get_disk()) {
    //         puts("Failed to open disk\n");
    //         return 1;
    //     }
    // }

    char   data[513];
    size_t steps = count / 512;
    if (count % 512) {
        steps++;
    }
    for (size_t i = 0; i < steps; i++) {
        size_t to_read = count;
        if (to_read > 512) {
            to_read = 512;
        }
        size_t read = disk_read(kernel_get_disk(), data, to_read, pos);
        data[read]  = 0;
        print_hexblock(data, read, 512 * i);
        count -= to_read;
        pos += to_read;
    }
    putc('\n');
    return 0;
}

static int disk_write_cmd(size_t argc, char ** argv) {
    // if (!kernel_get_disk()) {
    //     disk = disk_open(0, DISK_DRIVER_ATA);
    //     if (!kernel_get_disk()) {
    //         puts("Failed to open disk\n");
    //         return 1;
    //     }
    // }

    char data[512] = {0};
    for (size_t i = 0; i < 512; i++) {
        if ((i >> 4) < 10) {
            data[i * 2] = (i >> 4) + '0';
        }
        else {
            data[i * 2] = (i >> 4) + 'a' - 10;
        }

        if ((i & 0xf) < 10) {
            data[i * 2 + 1] = (i & 0xf) + '0';
        }
        else {
            data[i * 2 + 1] = (i & 0xf) + 'a' - 10;
        }
    }
    disk_write(kernel_get_disk(), data, 512, 0);
    return 0;
}

static int disk_size_cmd(size_t argc, char ** argv) {
    // if (!kernel_get_disk()) {
    //     disk = disk_open(0, DISK_DRIVER_ATA);
    //     if (!kernel_get_disk()) {
    //         puts("Failed to open disk\n");
    //         return 1;
    //     }
    // }
    size_t size = disk_size(kernel_get_disk());
    printf("Disk size %u\n", size);
    return 0;
}

static int currdir(size_t argc, char ** argv) {
    printf("Current dir is %p\n", mmu_get_curr_dir());
    return 0;
}

static int currproc(size_t argc, char ** argv) {
    printf("Current pid is %p\n", get_current_process()->pid);
    return 0;
}

static int hotswap(size_t argc, char ** argv) {
    uint32_t curr_dir = mmu_get_curr_dir();

    uint32_t addr = ram_page_alloc();
    if (!addr) {
        printf("Failed to allocate page\n");
        return 1;
    }

    mmu_dir_t * dir = paging_temp_map(addr);
    if (!dir) {
        printf("Failed to temp map\n");
        ram_page_free(addr);
        return 2;
    }

    mmu_dir_clear(dir);
    mmu_dir_set(dir, 0, mmu_dir_get_addr(UINT2PTR(VADDR_KERNEL_DIR), 0), MMU_DIR_RW);

    kmemcpy(dir, UINT2PTR(VADDR_KERNEL_DIR), sizeof(mmu_dir_t));

    printf("Switching to new dir %p\n", addr);

    mmu_change_dir(addr);

    paging_temp_free(addr);

    if (curr_dir != 0x1000) {
        ram_page_free(curr_dir);
    }

    return 0;
}

typedef struct {
    uint32_t addr;
    int      dir_i;
    int      table_i;
} little_t;

static void assert_all_unique(uint32_t cr3) {
    printf("Checking cr3 %p\n", cr3);

    little_t little;

    arr_t array;
    arr_create(&array, 1024, sizeof(little_t));
    little.addr    = cr3;
    little.dir_i   = -1;
    little.table_i = -1;
    arr_insert(&array, arr_size(&array), &little);

    mmu_dir_t * dir = paging_temp_map(cr3);

    for (size_t i = 0; i < MMU_DIR_SIZE; i++) {
        if (mmu_dir_get_flags(dir, i) & MMU_DIR_FLAG_PRESENT) {
            uint32_t      table_addr = mmu_dir_get_addr(dir, i);
            mmu_table_t * table      = paging_temp_map(table_addr);

            little.addr    = table_addr;
            little.dir_i   = i;
            little.table_i = -1;
            arr_insert(&array, arr_size(&array), &little);

            for (size_t j = 0; j < MMU_TABLE_SIZE; j++) {
                if (mmu_table_get_flags(table, j) & MMU_TABLE_FLAG_PRESENT) {
                    uint32_t page_addr = mmu_table_get_addr(table, j);

                    little.addr    = page_addr;
                    little.dir_i   = i;
                    little.table_i = j;
                    arr_insert(&array, arr_size(&array), &little);
                }
            }

            paging_temp_free(table_addr);
        }
    }

    paging_temp_free(cr3);

    puts("Checking pages\n");

    for (size_t i = 0; i < arr_size(&array); i++) {
        little_t * curr = arr_at(&array, i);

        for (size_t j = i + 1; j < arr_size(&array); j++) {
            little_t * search = arr_at(&array, j);

            if (curr->addr == search->addr) {
                printf("Found duplicate page %p src %i:%i dest %i:%i\n", curr->addr, curr->dir_i, curr->table_i, search->dir_i, search->table_i);
            }
        }
    }

    arr_free(&array);
}

static void assert_process_is(process_t * proc) {
    printf("Checking process %p\n", proc->cr3);

    mmu_dir_t * dir = paging_temp_map(proc->cr3);
    printf("Temp map to %p\n", dir);

    for (size_t i = 0; i < MMU_DIR_SIZE; i++) {
        if (mmu_dir_get_flags(dir, i) & MMU_DIR_FLAG_PRESENT) {
            uint32_t      table_addr = mmu_dir_get_addr(dir, i);
            mmu_table_t * table      = paging_temp_map(table_addr);

            printf("Entry %u present with address %p\n", i, table_addr);
            size_t page_count = 0;
            size_t are_zero   = 0;
            for (size_t j = 0; j < MMU_TABLE_SIZE; j++) {
                if (mmu_table_get_flags(table, j) & MMU_TABLE_FLAG_PRESENT) {
                    uint32_t page_addr = mmu_table_get_addr(table, j);
                    page_count++;
                    if (!page_addr) {
                        are_zero++;
                    }
                    else {
                        printf("%p ", page_addr);
                    }
                }
            }
            printf("Found %u pages with %u zeros\n", page_count, are_zero);

            paging_temp_free(table_addr);
        }
    }

    paging_temp_free(proc->cr3);
}

static int procswap(size_t argc, char ** argv) {
    static process_t * proc = 0;

    process_t * next_proc = kmalloc(sizeof(process_t));

    if (process_create(next_proc)) {
        printf("Failed to do it\n");
        kfree(next_proc);
        return 1;
    }

    assert_all_unique(next_proc->cr3);
    assert_process_is(next_proc);

    // void * ptr1 = process_add_pages(next_proc, 1);

    mmu_change_dir(next_proc->cr3);

    // void * ptr2 = process_add_pages(next_proc, 1);

    if (proc) {
        printf("Free first\n");
        process_free(proc);
        kfree(proc);
    }

    // printf("Ptr1 = %p\n", ptr1);
    // printf("Ptr2 = %p\n", ptr2);

    proc = next_proc;

    // assert_process_is(proc);

    return 0;
}

static int command_lookup(size_t argc, char ** argv) {
    char * filename = argv[0];

    if (!kernel_get_tar()) {
        puts("Filesystem not mounted\n");
        return 1;
    }

    tar_stat_t stat;
    if (!tar_stat_file(kernel_get_tar(), filename, &stat)) {
        puts("Failed to find file\n");
        return 1;
    }

    uint8_t * buff = kmalloc(stat.size);
    if (!buff) {
        return 1;
    }

    tar_fs_file_t * file = tar_file_open(kernel_get_tar(), filename);
    if (!file) {
        kfree(buff);
        return 1;
    }

    if (!tar_file_read(file, buff, stat.size)) {
        tar_file_close(file);
        kfree(buff);
        return 1;
    }

    int res = command_exec(buff, filename, stat.size, argc, argv);

    if (!buff) {
        return 0;
    }

    tar_file_close(file);
    kfree(buff);

    return res;
}

void commands_init() {
    // set_command_lookup(command_lookup);

    // term_command_add("currdir", currdir);
    // term_command_add("pid", currproc);
    // term_command_add("hotswap", hotswap);
    // term_command_add("procswap", procswap);

    // term_command_add("clear", clear_cmd);
    // term_command_add("echo", echo_cmd);
    // term_command_add("debug", debug_cmd);
    // term_command_add("atoi", atoi_cmd);
    // term_command_add("outb", port_out_cmd);
    // term_command_add("inb", port_in_cmd);
    // term_command_add("time", time_cmd);
    // term_command_add("sleep", sleep_cmd);
    // term_command_add("ret", ret_cmd);
    // // term_command_add("format", format_cmd);
    // // term_command_add("mount", mount_cmd);
    // // term_command_add("unmount", unmount_cmd);
    // // term_command_add("mem", mem_cmd);
    // term_command_add("ls", ls_cmd);
    // term_command_add("stat", stat_cmd);
    // // term_command_add("status", status_cmd);
    // term_command_add("read", fs_read_cmd);
    // term_command_add("cat", fs_cat_cmd);
    // // term_command_add("read", disk_read_cmd);
    // term_command_add("write", disk_write_cmd);
    // term_command_add("size", disk_size_cmd);
}
