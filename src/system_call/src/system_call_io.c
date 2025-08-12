#include "kernel/system_call_io.h"

#include "io/file.h"
#include "kernel.h"
#include "libc/datastruct/array.h"
#include "libk/defs.h"
#include "process.h"

static handle_t * get_free_handle(process_t * proc);

int sys_call_io_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    process_t * proc       = get_current_process();
    arr_t *     io_handles = &proc->io_handles;

    switch (int_no) {
        case SYS_INT_IO_OPEN: {
            struct _args {
                const char * path;
                const char * mode;
            } * args = (struct _args *)args_data;

            if (!args->path || !args->mode || !*args->path || !*args->mode) {
                return 0;
            }

            handle_t * handle = get_free_handle(proc);

            if (!handle) {
                return 0;
            }

            handle->type = HANDLE_TYPE_FILE; // TODO type by path prefix
            return handle->id;
        } break;

        case SYS_INT_IO_CLOSE: {
            struct _args {
                int handle;
            } * args = (struct _args *)args_data;

            if (args->handle > arr_size(io_handles)) {
                return 0; // TODO proper error
            }

            handle_t * handle = arr_at(io_handles, args->handle - 1);

            if (handle->type == HANDLE_TYPE_FREE) {
                return 0; // TODO proper error
            }

            handle->type = HANDLE_TYPE_FREE;

            return 0;
        } break;

        case SYS_INT_IO_READ: {
            struct _args {
                int    handle;
                char * buff;
                size_t count;
            } * args = (struct _args *)args_data;

            if (args->handle > arr_size(io_handles)) {
                return 0; // TODO proper error
            }

            handle_t * handle = arr_at(io_handles, args->handle - 1);

            if (handle->type == HANDLE_TYPE_FREE) {
                return 0; // TODO proper error
            }

            // TODO
        } break;

        case SYS_INT_IO_WRITE: {
            struct _args {
                int          handle;
                const char * buff;
                size_t       count;
            } * args = (struct _args *)args_data;

            if (args->handle > arr_size(io_handles)) {
                return 0; // TODO proper error
            }

            handle_t * handle = arr_at(io_handles, args->handle - 1);

            if (handle->type == HANDLE_TYPE_FREE) {
                return 0; // TODO proper error
            }

            // TODO
        } break;

        case SYS_INT_IO_SEEK: {
            struct _args {
                int handle;
                int pos;
                int seek;
            } * args = (struct _args *)args_data;

            if (args->handle > arr_size(io_handles)) {
                return 0; // TODO proper error
            }

            handle_t * handle = arr_at(io_handles, args->handle - 1);

            if (handle->type == HANDLE_TYPE_FREE) {
                return 0; // TODO proper error
            }

            // TODO
        } break;

        case SYS_INT_IO_TELL: {
            struct _args {
                int handle;
            } * args = (struct _args *)args_data;

            if (args->handle > arr_size(io_handles)) {
                return 0; // TODO proper error
            }

            handle_t * handle = arr_at(io_handles, args->handle - 1);

            if (handle->type == HANDLE_TYPE_FREE) {
                return 0; // TODO proper error
            }

            // TODO
        } break;
    }

    return 0;
}

static handle_t * get_free_handle(process_t * proc) {
    arr_t * io_handles = &proc->io_handles;

    for (size_t i = 0; i < arr_size(io_handles); i++) {
        handle_t * handle = arr_at(io_handles, i);

        if (handle->type == HANDLE_TYPE_FREE) {
            return handle;
        }
    }

    handle_t new_handle;
    new_handle.id   = arr_size(io_handles) + 1; // index at 1
    new_handle.type = HANDLE_TYPE_FREE;

    if (arr_insert(io_handles, arr_size(io_handles), &new_handle)) {
        return 0;
    }

    return arr_at(io_handles, arr_size(io_handles) - 1);
}
