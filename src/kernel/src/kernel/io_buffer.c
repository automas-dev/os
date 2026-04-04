#include "kernel/io_buffer.h"

#include "libc/memory.h"

io_buffer_t * io_buffer_create(size_t size) {
    if (!size) {
        return 0;
    }

    io_buffer_t * buff = pmalloc(sizeof(io_buffer_t));
    if (buff) {
        if (cb_create(&buff->buff, size, 1)) {
            pfree(buff);
            return 0;
        }
    }
    return buff;
}

void io_buffer_free(io_buffer_t * buff) {
    if (!buff) {
        return;
    }

    cb_free(&buff->buff);
    pfree(buff);
}

int io_buffer_push(io_buffer_t * buff, char c) {
    if (!buff) {
        return -1;
    }

    cb_t * cb = &buff->buff;

    if (cb_len(cb) == cb_buff_size(cb)) {
        if (cb_pop(cb, 0)) {
            return -1;
        }
    }

    if (cb_push(cb, &c)) {
        return -1;
    }

    return 0;
}

int io_buffer_pop(io_buffer_t * buff, char * c_out) {
    if (!buff) {
        return -1;
    }

    cb_t * cb = &buff->buff;

    if (cb_len(cb) == 0) {
        return -1;
    }

    return cb_pop(cb, c_out);
}

size_t io_buffer_size(const io_buffer_t * buff) {
    if (!buff) {
        return 0;
    }

    return cb_buff_size(&buff->buff);
}

size_t io_buffer_length(const io_buffer_t * buff) {
    if (!buff) {
        return 0;
    }

    return cb_len(&buff->buff);
}
