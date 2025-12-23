#include "drivers/vga.h"

#include "cpu/ports.h"
#include "defs.h"

#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

#define MAX_INDEX (VGA_ROWS * VGA_COLS)

static int    index;
static char   color;
static char * screen;

static void update_cursor();
static void shift_lines();

void vga_init(char * ptr) {
    index  = 0;
    color  = VGA_RESET;
    screen = ptr;

    vga_clear();
}

/*
 * DIRECT ACCESS
 */

void vga_clear() {
    for (int row = 0; row < VGA_ROWS; row++) {
        for (int col = 0; col < VGA_COLS; col++) {
            index = VGA_INDEX(row, col);
            vga_put(index, ' ', VGA_RESET);
        }
    }
    index = 0;
    color = VGA_RESET;
}

void vga_put(int index, char c, unsigned char attr) {
    index *= 2;
    screen[index]     = c;
    screen[index + 1] = attr;
}

/*
 * MANAGED ACCESS
 */

int vga_cursor_row() {
    return VGA_ROW(index);
}

int vga_cursor_col() {
    return VGA_COL(index);
}

int vga_index() {
    return index;
}

void vga_cursor(int row, int col) {
    if (row < 0 || col < 0 || row >= VGA_ROWS || col >= VGA_COLS) {
        return;
    }

    index = VGA_INDEX(row, col);
    update_cursor();
}

void vga_cursor_hide() {
    port_byte_out(REG_SCREEN_CTRL, 0x0a);
    port_byte_out(REG_SCREEN_DATA, 0x3f);
}

void vga_cursor_show() {
    port_byte_out(REG_SCREEN_CTRL, 0x0a);
    port_byte_out(REG_SCREEN_DATA, (port_byte_in(REG_SCREEN_DATA) & 0xc0) | 0xd);

    port_byte_out(REG_SCREEN_CTRL, 0x0b);
    port_byte_out(REG_SCREEN_DATA, (port_byte_in(REG_SCREEN_DATA) & 0xe0) | 0xe);
}

/*
 * HIGH LEVEL
 */

void vga_color(unsigned char attr) {
    color = attr;
}

size_t vga_putc(char c) {
    size_t ret = 0;
    if (c == '\n') {
        int row = VGA_ROW(index);
        index   = VGA_INDEX(row + 1, 0);
        ret     = 0;
    }
    else if (c == '\b') {
        if (index > 0) {
            index--;
        }
        vga_put(index, ' ', VGA_RESET);
    }
    else {
        vga_put(index++, c, color);
        ret = 1;
    }

    if (index >= MAX_INDEX) {
        shift_lines();
        index = VGA_INDEX(VGA_ROWS - 1, 0);
    }

    update_cursor();
    return ret;
}

size_t vga_puts(const char * str) {
    if (!str) {
        return 0;
    }

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
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(index >> 8));
    port_byte_out(REG_SCREEN_CTRL, 15);
    port_byte_out(REG_SCREEN_DATA, (unsigned char)(index & 0xff));
}

static void shift_lines() {
    for (size_t i = 0; i < ((VGA_ROWS - 1) * VGA_COLS * 2); i++) {
        screen[i] = screen[i + VGA_COLS * 2];
    }

    for (int col = 0; col < VGA_COLS; col++) {
        int index = VGA_INDEX(VGA_ROWS - 1, col);
        vga_put(index, ' ', VGA_RESET);
    }
}
