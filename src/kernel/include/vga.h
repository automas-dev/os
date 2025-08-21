#ifndef DRIVER_VGA_H
#define DRIVER_VGA_H

#include <stddef.h>
#include <stdint.h>

#define VGA_ROWS           25
#define VGA_COLS           80
#define VGA_RESET          (VGA_FG_LIGHT_GRAY | VGA_BG_BLACK)
#define VGA_WHITE_ON_BLACK (VGA_FG_WHITE | VGA_BG_BLACK)
#define VGA_RED_ON_WHITE   (VGA_FG_RED | VGA_BG_WHITE)

#define VGA_ROW(INDEX)      ((INDEX) / VGA_COLS)
#define VGA_COL(INDEX)      ((INDEX) % VGA_COLS)
#define VGA_INDEX(ROW, COL) (((ROW) * VGA_COLS) + (COL))

enum VGA_FG {
    VGA_FG_BLACK         = 0x0,
    VGA_FG_BLUE          = 0x1,
    VGA_FG_GREEN         = 0x2,
    VGA_FG_CYAN          = 0x3,
    VGA_FG_RED           = 0x4,
    VGA_FG_MAGENTA       = 0x5,
    VGA_FG_BROWN         = 0x6,
    VGA_FG_LIGHT_GRAY    = 0x7,
    VGA_FG_DARK_GRAY     = 0x8,
    VGA_FG_LIGHT_BLUE    = 0x9,
    VGA_FG_LIGHT_GREEN   = 0xA,
    VGA_FG_LIGHT_CYAN    = 0xB,
    VGA_FG_LIGHT_RED     = 0xC,
    VGA_FG_LIGHT_MAGENTA = 0xD,
    VGA_FG_LIGHT_BROWN   = 0xE,
    VGA_FG_WHITE         = 0xF,
};

enum VGA_BG {
    VGA_BG_BLACK         = 0x00,
    VGA_BG_BLUE          = 0x10,
    VGA_BG_GREEN         = 0x20,
    VGA_BG_CYAN          = 0x30,
    VGA_BG_RED           = 0x40,
    VGA_BG_MAGENTA       = 0x50,
    VGA_BG_BROWN         = 0x60,
    VGA_BG_LIGHT_GRAY    = 0x70,
    VGA_BG_DARK_GRAY     = 0x80,
    VGA_BG_LIGHT_BLUE    = 0x90,
    VGA_BG_LIGHT_GREEN   = 0xA0,
    VGA_BG_LIGHT_CYAN    = 0xB0,
    VGA_BG_LIGHT_RED     = 0xC0,
    VGA_BG_LIGHT_MAGENTA = 0xD0,
    VGA_BG_LIGHT_BROWN   = 0xE0,
    VGA_BG_WHITE         = 0xF0,
};

/**
 * @brief Setup VGA driver with vga memory address.
 *
 * The cursor / index will also be reset to 0 and the color will be reset to
 * white on black.
 */
void vga_init();

/**
 * @brief Clear the VGA buffer and reset the cursor and color.
 */
void vga_clear();

/**
 * @brief Put a single character on the screen with fg / bg colors (attr).
 *
 * @param index vga index of character
 * @param c character to put
 * @param attr foreground and background colors
 */
void vga_put(int index, char c, unsigned char attr);

/**
 * @brief Get the current cursor row.
 *
 * @return int cursor row
 */
int vga_cursor_row();

/**
 * @brief Get the current cursor column.
 *
 * @return int cursor column
 */
int vga_cursor_col();

/**
 * @brief Get the current vga index.
 *
 * The index is used to calculate column and row using VGA_COLS.
 *
 * @return int vga index
 */
int vga_index();

/**
 * @brief Move the cursor to the position row / col.
 *
 * @param row cursor row
 * @param col cursor column
 */
void vga_cursor(int row, int col);

/**
 * @brief Hide the cursor.
 */
void vga_cursor_hide();

/**
 * @brief Show the cursor.
 */
void vga_cursor_show();

/**
 * @brief Set the color.
 *
 * @param color foreground and background colors ored (|) together.
 */
void vga_color(unsigned char color);

/**
 * @brief Write a single character to the buffer and increment the cursor.
 *
 * @param c character to write
 * @return size_t number of characters written (0 for failure)
 */
size_t vga_putc(char c);

/**
 * @brief Write a string of characters to the buffer and increment the cursor.
 *
 * @param str characters to write
 * @return size_t number of characters written (0 for failure)
 */
size_t vga_puts(const char * str);

/**
 * @brief Write a signed integer to the buffer and increment the cursor.
 *
 * @param num integer to write
 * @return size_t number of characters written (0 for failure)
 */
size_t vga_puti(int num);

/**
 * @brief Write an unsigned integer to the buffer and increment the cursor
 *
 * @param num integer to write
 * @return size_t number of characters written (0 for failure)
 */
size_t vga_putu(unsigned int num);

/**
 * @brief Write a hexadecimal number to the buffer and increment the cursor.
 *
 * Letters are written in upper case. No leading "0x" is included. The user must
 * write this if desired.
 *
 * @param num integer to write
 * @return size_t number of characters written (0 for failure)
 */
size_t vga_putx(unsigned int num);

size_t vga_write(const char * buff, size_t size);

#endif // DRIVER_VGA_H
