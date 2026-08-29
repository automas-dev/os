#define KLOG_SERVICE "KERNEL/IO_BUFFER"

#include "kernel/io/buffer.h"
#include "kernel/logs.h"
#include "libc/memory.h"

io_buffer_t * io_buffer_create(size_t size) {
    if (!size) {
        KLOG_WARNING("io_buffer_create received a size of 0");
        return 0;
    }

    io_buffer_t * buff = pmalloc(sizeof(io_buffer_t));
    if (buff) {
        if (cb_create(&buff->buff, size, 1)) {
            KLOG_ERROR("io_buffer_create failed to create circular buffer of size %u", size);
            pfree(buff);
            return 0;
        }
        KLOG_TRACE("Created io buffer %p of size %u", buff, size);
    }

    return buff;
}

void io_buffer_free(io_buffer_t * buff) {
    if (!buff) {
        KLOG_WARNING("io_buffer_free received a null pointer for the buffer struct");
        return;
    }

    cb_free(&buff->buff);
    pfree(buff);
}

int io_buffer_push(io_buffer_t * buff, char c) {
    if (!buff) {
        KLOG_WARNING("io_buffer_push received a null pointer for the buffer struct");
        return -1;
    }

    cb_t * cb = &buff->buff;

    if (cb_len(cb) == cb_buff_size(cb)) {
        if (cb_pop(cb, 0)) {
            KLOG_ERROR("Failed to pop element 0 from circular buffer %p of io buffer %p", cb, buff);
            return -1;
        }
    }

    if (cb_push(cb, &c)) {
        KLOG_ERROR("Failed to push character 0x%02X (%c) to circular buffer %p of io buffer %p", c, c, cb, buff);
        return -1;
    }

    return 0;
}

int io_buffer_pop(io_buffer_t * buff, char * c_out) {
    if (!buff) {
        KLOG_WARNING("io_buffer_pop received a null pointer for the buffer struct");
        return -1;
    }
    // c_out can be null

    cb_t * cb = &buff->buff;

    if (cb_len(cb) == 0) {
        KLOG_WARNING("Tried to pop char from empty to circular buffer %p of io buffer %p", cb, buff);
        return -1;
    }

    return cb_pop(cb, c_out);
}

size_t io_buffer_size(const io_buffer_t * buff) {
    if (!buff) {
        KLOG_WARNING("io_buffer_size received a null pointer for the buffer struct");
        return 0;
    }

    return cb_buff_size(&buff->buff);
}

size_t io_buffer_length(const io_buffer_t * buff) {
    if (!buff) {
        KLOG_WARNING("io_buffer_length received a null pointer for the buffer struct");
        return 0;
    }

    return cb_len(&buff->buff);
}
