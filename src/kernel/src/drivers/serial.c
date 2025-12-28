#include "drivers/serial.h"

#include "cpu/ports.h"
#include "libc/string.h"

// WARNING serial driver is used in logging, so be careful about where you log
// so you don't get an infinite loop.

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

static int serial_received(uint16_t port) {
    return port_byte_in(port + 5) & 1;
}

char serial_read(uint16_t port) {
    while (serial_received(port) == 0);

    return port_byte_in(port);
}

static int is_transmit_empty(uint16_t port) {
    return port_byte_in(port + 5) & 0x20;
}

void serial_write_str(uint16_t port, const char * str) {
    if (!str) {
        return;
    }

    size_t count = kstrlen(str);
    serial_write(port, str, count);
}

void serial_write(uint16_t port, const char * str, size_t count) {
    if (!str) {
        return;
    }

    while (is_transmit_empty(port) == 0);

    for (size_t i = 0; i < count; i++) {
        port_byte_out(port, *str++);
    }
}
