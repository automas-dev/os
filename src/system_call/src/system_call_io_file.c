#include "kernel/system_call_io_file.h"

#include "io/file.h"
#include "kernel.h"
#include "libc/datastruct/array.h"
#include "libk/defs.h"
#include "process.h"

static handle_t * get_free_handle(process_t * proc);
static handle_t * find_handler(process_t * proc, int handle);

int sys_call_io_file_cb(uint16_t int_no, void * args_data, registers_t * regs) {
    process_t * proc       = get_current_process();
    arr_t *     io_handles = &proc->io_handles;

    switch (int_no) {
        case SYS_INT_IO_FILE_OPEN: {
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

            handle->type   = HANDLE_TYPE_FILE; // TODO type by path prefix
            handle->cursor = 0;

            return handle->id;
        } break;

        case SYS_INT_IO_FILE_CLOSE: {
            struct _args {
                int handle;
            } * args = (struct _args *)args_data;

            handle_t * handle = find_handler(proc, args->handle);

            if (!handle) {
                return 0;
            }

            handle->type = HANDLE_TYPE_FREE;

            return 0;
        } break;

        case SYS_INT_IO_FILE_READ: {
            struct _args {
                int    handle;
                size_t size;
                size_t count;
                void * buff;
            } * args = (struct _args *)args_data;

            if (args->size == 0 || args->count == 0 || !args->buff) {
                return 0;
            }

            handle_t * handle = find_handler(proc, args->handle);

            if (!handle) {
                return 0;
            }

            // TODO ACTUALLY DO THE READ
        } break;

        case SYS_INT_IO_FILE_WRITE: {
            struct _args {
                int          handle;
                size_t       size;
                size_t       count;
                const void * buff;
            } * args = (struct _args *)args_data;

            if (args->size == 0 || args->count == 0 || !args->buff) {
                return 0;
            }

            handle_t * handle = find_handler(proc, args->handle);

            if (!handle) {
                return 0;
            }

            // TODO ACTUALLY DO THE WRITE
            KPANIC("File write is not yet implemented");
        } break;

        case SYS_INT_IO_FILE_SEEK: {
            struct _args {
                int handle;
                int pos;
                int seek;
            } * args = (struct _args *)args_data;

            handle_t * handle = find_handler(proc, args->handle);

            if (!handle) {
                return 0;
            }

            // TODO ACTUALLY DO THE SEEK
        } break;

        case SYS_INT_IO_FILE_TELL: {
            struct _args {
                int handle;
            } * args = (struct _args *)args_data;

            handle_t * handle = find_handler(proc, args->handle);

            if (!handle) {
                return 0;
            }

            // TODO ACTUALLY DO THE TELL
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
    new_handle.id   = arr_size(io_handles) + 1;
    new_handle.type = HANDLE_TYPE_FREE;

    if (arr_insert(io_handles, arr_size(io_handles), &new_handle)) {
        return 0;
    }

    return arr_at(io_handles, arr_size(io_handles) - 1);
}

static handle_t * find_handler(process_t * proc, int handle_id) {
    if (!proc || handle_id <= 0) {
        return 0;
    }

    arr_t * io_handles = &proc->io_handles;

    if (handle_id > arr_size(io_handles)) {
        return 0;
    }

    handle_t * handle = arr_at(io_handles, handle_id - 1);

    if (handle->type != HANDLE_TYPE_FILE) {
        return 0;
    }

    return handle;
}
