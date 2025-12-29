#include "drivers/vga.h"

#include "cpu/ports.h"
#include "defs.h"
#include "kernel/logs.h"

// WARNING vga driver was previously used in logging, so be careful using it for
// log outputs.

#undef SERVICE
#define SERVICE "DRIVER/VGA"

#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

#define MAX_INDEX (VGA_ROWS * VGA_COLS)

static int    __index;
static char   __color;
static char * __screen;

static void update_cursor();
static void shift_lines();

void vga_init(char * ptr) {
    __index  = 0;
    __color  = VGA_RESET;
    __screen = ptr;

    KLOG_DEBUG("Initialized driver");

    vga_clear();
}

/*
 * DIRECT ACCESS
 */

void vga_clear() {
    KLOG_TRACE("Clear screen");

    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            __index = VGA_INDEX(row, col);
            vga_put(__index, ' ', VGA_RESET);
        }
    }
    __index = 0;
    __color = VGA_RESET;
}

void vga_put(int index, char c, unsigned char attr) {
    // idk if this is too much
    KLOG_TRACE("Put character 0x%X (%c) to index %d with attr 0x%X", c, c, index, attr);
    index *= 2;
    __screen[index]     = c;
    __screen[index + 1] = attr;
}

/*
 * MANAGED ACCESS
 */

int vga_cursor_row() {
    return VGA_ROW(__index);
}

int vga_cursor_col() {
    return VGA_COL(__index);
}

int vga_index() {
    return __index;
}

void vga_cursor(int row, int col) {
    if (row < 0 || col < 0 || row >= VGA_ROWS || col >= VGA_COLS) {
        return;
    }

    __index = VGA_INDEX(row, col);
    KLOG_TRACE("Set cursor to row %d col %d which is index %d", row, col, __index);

    update_cursor();
}

void vga_cursor_hide() {
    KLOG_TRACE("Hiding cursor");

    port_byte_out(REG_SCREEN_CTRL, 0x0a);
    port_byte_out(REG_SCREEN_DATA, 0x3f);
}

void vga_cursor_show() {
    KLOG_TRACE("Showing cursor");

    port_byte_out(REG_SCREEN_CTRL, 0x0a);
    port_byte_out(REG_SCREEN_DATA, (port_byte_in(REG_SCREEN_DATA) & 0xc0) | 0xd);

    port_byte_out(REG_SCREEN_CTRL, 0x0b);
    port_byte_out(REG_SCREEN_DATA, (port_byte_in(REG_SCREEN_DATA) & 0xe0) | 0xe);
}

/*
 * HIGH LEVEL
 */

void vga_color(unsigned char attr) {
    KLOG_TRACE("Setting color to %X", attr);
    __color = attr;
}

size_t vga_putc(char c) {
    // Not much logging needed because of trace in vga_put
    size_t ret = 0;
    if (c == '\n') {
        int row = VGA_ROW(__index);
        __index = VGA_INDEX(row + 1, 0);
        ret     = 0;
    }
    else if (c == '\b') {
        if (__index > 0) {
            __index--;
        }
        vga_put(__index, ' ', VGA_RESET);
    }
    else {
        vga_put(__index++, c, __color);
        ret = 1;
    }

    if (__index >= MAX_INDEX) {
        shift_lines();
        __index = VGA_INDEX(VGA_ROWS - 1, 0);
    }

    update_cursor();
    return ret;
}

size_t vga_puts(const char * str) {
    if (!str) {
        KLOG_ERROR("Tried to put null pointer string");
        return 0;
    }

    // Not much logging needed because of trace in vga_put
    size_t len = 0;
    while (*str != 0) {
        vga_putc(*str++);
        len++;
    }
    return len;
}

static char digit(uint32_t num, uint8_t base) {
    if (num < 10) {
        return num + '0';
    }
    else {
        return (num - 10) + 'A';
    }
}

static size_t _print_uint(uint32_t num, uint8_t base) {
    if (num == 0) {
        return vga_putc('0');
    }

    size_t   len = 0;
    uint32_t rev = 0;
    while (num > 0) {
        rev = (rev * base) + (num % base);
        num /= base;
        len++;
    }

    size_t o_len = 0;
    for (size_t i = 0; i < len; i++) {
        o_len += vga_putc(digit(rev % base, base));
        rev /= base;
    }

    return o_len;
}

size_t vga_puti(int num) {
    size_t o_len = 0;
    if (num < 0) {
        o_len += vga_putc('-');
        num = -num;
    }
    o_len += _print_uint(num, 10);
    return o_len;
}

size_t vga_putu(unsigned int num) {
    _print_uint(num, 10);
}

size_t vga_putx(unsigned int num) {
    return _print_uint(num, 16);
}

size_t vga_write(const char * buff, size_t size) {
    for (size_t i = 0; i < size; i++) {
        vga_putc(buff[i]);
    }
    return size;
}

/*
 * HELPER FUNCTIONS
 */

static void update_cursor() {
    port_byte_out(REG_SCREEN_CTRL, 14);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(__index >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(__index & 0xff));
}

static void shift_lines() {
    KLOG_TRACE("Shifting lines");
    for (size_t i = 0; i < ((VGA_ROWS - 1) * VGA_COLS * 2); i++) {
        __screen[i] = __screen[i + VGA_COLS * 2];
    }

    for (int col = 0; col < VGA_COLS; col++) {
        int index = VGA_INDEX(VGA_ROWS - 1, col);
        vga_put(index, ' ', VGA_RESET);
    }
}
