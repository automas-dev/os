#include "kernel/system_call_stdio.h"

#include <stddef.h>

#include "defs.h"
#include "drivers/vga.h"
#include "libk/defs.h"

int sys_call_tmp_stdio_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    int res = 0;

    switch (int_no) {
        case SYS_INT_STDIO_PUTC: {
            struct _args {
                char c;
            } * args = (struct _args *)args_data;
            res      = vga_putc(args->c);
        } break;

        case SYS_INT_STDIO_PUTS: {
            struct _args {
                const char * str;
            } * args = (struct _args *)args_data;
            res      = vga_puts(args->str);
        } break;
    }

    return 0;
}
