#include "drivers/serial.h"

#include "cpu/ports.h"
#include "libc/string.h"

// WARNING serial driver is used in logging, so be careful about where you log
// so you don't get an infinite loop.

// TODO might need to disable interrupts while interacting with ports?

static int serial_received(uint16_t port);
static int is_transmit_empty(uint16_t port);

int serial_init(uint16_t port) {
    port_byte_out(port + 1, 0x00); // Disable all interrupts
    port_byte_out(port + 3, 0x80); // Enable DLAB (set baud rate divisor)
    port_byte_out(port + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    port_byte_out(port + 1, 0x00); //                  (hi byte)
    port_byte_out(port + 3, 0x03); // 8 bits, no parity, one stop bit
    port_byte_out(port + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
    port_byte_out(port + 4, 0x0B); // IRQs enabled, RTS/DSR set
    port_byte_out(port + 4, 0x1E); // Set in loopback mode, test the serial chip
    port_byte_out(port + 0, 0xAE); // Test serial chip (send byte 0xAE and check if serial returns same byte)

    // Check if serial is faulty (i.e: not same byte as sent)
    if (port_byte_in(port + 0) != 0xAE) {
        return 1;
    }

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    port_byte_out(port + 4, 0x0F);
    return 0;
}

char serial_read_char(uint16_t port) {
    while (serial_received(port) == 0);

    return port_byte_in(port);
}

size_t serial_read(uint16_t port, char * buff, size_t count) {
    if (!buff) {
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        buff[i] = serial_read_char(port);
    }

    return count;
}

size_t serial_write_char(uint16_t port, char c) {
    while (is_transmit_empty(port) == 0);

    port_byte_out(port, c);

    return 1;
}

size_t serial_write_str(uint16_t port, const char * str) {
    if (!str) {
        return 0;
    }

    size_t count = kstrlen(str);
    return serial_write(port, str, count);
}

size_t serial_write(uint16_t port, const char * str, size_t count) {
    if (!str) {
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        serial_write_char(port, str[i]);
    }

    return count;
}

static int serial_received(uint16_t port) {
    return port_byte_in(port + 5) & 1;
}

static int is_transmit_empty(uint16_t port) {
    return port_byte_in(port + 5) & 0x20;
}
