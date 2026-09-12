#ifndef DRIVERS_SERIAL_H
#define DRIVERS_SERIAL_H

#include <stddef.h>
#include <stdint.h>

// See https://wiki.osdev.org/Serial_Ports

enum SERIAL_PORT {
    SERIAL_PORT_COM1 = 0x3F8,
    SERIAL_PORT_COM2 = 0x2F8,
    SERIAL_PORT_COM3 = 0x3E8,
    SERIAL_PORT_COM4 = 0x2E8,
    SERIAL_PORT_COM5 = 0x5F8,
    SERIAL_PORT_COM6 = 0x4F8,
    SERIAL_PORT_COM7 = 0x5E8,
    SERIAL_PORT_COM8 = 0x4E8,
};

/**
 * @brief Setup the serial port for 38400 baud, 8 bits, no parity, one stop bit.
 *
 * The chip is tested in loopback mode before being put into normal operation.
 *
 * @param port serial port base address
 * @return 0 for success, 1 if the chip is faulty
 */
int serial_init(uint16_t port);

/**
 * @brief Read a single character, blocking until one is received.
 *
 * @param port serial port base address
 * @return character read
 */
char serial_read_char(uint16_t port);

/**
 * @brief Read count characters into buff, blocking until they are received.
 *
 * @param port serial port base address
 * @param buff buffer to read into
 * @param count number of characters to read
 * @return number of characters read (0 for failure)
 */
size_t serial_read(uint16_t port, char * buff, size_t count);

/**
 * @brief Write a single character, blocking until it can be transmitted.
 *
 * @param port serial port base address
 * @param c character to write
 * @return number of characters written (0 for failure)
 */
size_t serial_write_char(uint16_t port, char c);

/**
 * @brief Write a null terminated string.
 *
 * @param port serial port base address
 * @param str characters to write
 * @return number of characters written (0 for failure)
 */
size_t serial_write_str(uint16_t port, const char * str);

/**
 * @brief Write count characters from str.
 *
 * @param port serial port base address
 * @param str characters to write
 * @param count number of characters to write
 * @return number of characters written (0 for failure)
 */
size_t serial_write(uint16_t port, const char * str, size_t count);

#endif // DRIVERS_SERIAL_H
