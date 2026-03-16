#include "kernel/panic.h"

#include "drivers/vga.h"
#include "kernel/logs.h"

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
    KLOG_ERROR("[KERNEL PANIC][%s]:%u %s", file, line, msg);
    halt();
}

NO_RETURN void halt() {
    for (;;) {
        asm("cli");
        asm("hlt");
    }
}
