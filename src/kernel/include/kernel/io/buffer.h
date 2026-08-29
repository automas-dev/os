#ifndef KERNEL_IO_BUFFER_H
#define KERNEL_IO_BUFFER_H

#include <stddef.h>

#include "libc/datastruct/circular_buffer.h"

typedef struct _io_buffer {
    cb_t buff; // array<char>
} io_buffer_t;

io_buffer_t * io_buffer_create(size_t size);
void          io_buffer_free(io_buffer_t * buff);

int    io_buffer_push(io_buffer_t * buff, char c);
int    io_buffer_pop(io_buffer_t * buff, char * c_out);
size_t io_buffer_size(const io_buffer_t * buff);
size_t io_buffer_length(const io_buffer_t * buff);

#endif // KERNEL_IO_BUFFER_H
