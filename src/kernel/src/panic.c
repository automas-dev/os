#include "kernel.h"
#include "vga.h"

NO_RETURN void kernel_panic(const char * msg, const char * file, unsigned int line) {
    vga_color(VGA_FG_WHITE | VGA_BG_RED);
    vga_puts("[KERNEL PANIC]");
    if (file) {
        vga_putc('[');
        vga_puts(file);
        vga_puts("]:");
        vga_putu(line);
    }
    if (msg) {
        vga_putc(' ');
        vga_puts(msg);
    }
    vga_cursor_hide();
    halt();
}

NO_RETURN void halt() {
    for (;;) {
        asm("cli");
        asm("hlt");
    }
}
