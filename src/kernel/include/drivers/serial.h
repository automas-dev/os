#ifndef DRIVER_SERIAL_H
#define DRIVER_SERIAL_H

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

int serial_init(uint16_t port);

char serial_read(uint16_t port);
void serial_write_str(uint16_t port, const char * str);
void serial_write(uint16_t port, const char * str, size_t count);

#endif // DRIVER_SERIAL_H
