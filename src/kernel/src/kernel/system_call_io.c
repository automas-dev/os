#define KLOG_SERVICE "SYSCALL/IO"

#include "kernel/system_call_io.h"

#include "drivers/vga.h"
#include "kernel.h"
#include "kernel/device/fs_file.h"
#include "kernel/logs.h"
#include "libc/datastruct/array.h"
#include "libk/defs.h"
#include "process.h"

int sys_call_io_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    process_t * proc       = get_current_process();
    arr_t *     io_handles = &proc->io_handles;

    KLOG_TRACE("Call id 0x%X from pid %u", call_id, proc->pid);

    switch (call_id) {
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }
        case SYS_CALL_IO_OPEN: {
            struct _args {
                const char * path;
                const char * mode;
            } * args = (struct _args *)args_data;

            if (!args->path || !args->mode || !*args->path || !*args->mode) {
                return 0;
            }

            io_device_t * d = device_fs_file_open(args->path, args->mode);
            if (!d) {
                return 0;
            }

            return process_add_handle(proc, -1, 0, d);
        } break;

            // case SYS_CALL_IO_CLOSE: {
            //     struct _args {
            //         int handle;
            //     } * args = (struct _args *)args_data;

            //     if (args->handle > arr_size(io_handles)) {
            //         return 0; // TODO proper error
            //     }

            //     handle_t * handle = arr_at(io_handles, args->handle - 1);

            //     if (handle->type == HANDLE_TYPE_FREE) {
            //         return 0; // TODO proper error
            //     }

            //     handle->type = HANDLE_TYPE_FREE;

            //     return 0;
            // } break;

        case SYS_CALL_IO_READ: {
            struct _args {
                int    handle;
                char * buff;
                size_t count;
                size_t pos;
            } * args = (struct _args *)args_data;

            if (args->handle < 0 || !args->buff || !args->count) {
                return 0;
            }

            // TODO get stdin handle and use device read
            // TODO add buffer to handle_t

            if (args->handle == 0) {
                size_t available = io_buffer_length(proc->io_buffer);
                size_t count     = args->count; // idk if this needs to be copied or can be edited in place
                if (count > available) {
                    count = available;
                }

                if (count > 0) {
                    for (size_t i = 0; i < count; i++) {
                        if (io_buffer_pop(proc->io_buffer, &args->buff[i])) {
                            break;
                        }
                    }
                }

                return count;
            }

            handle_t * h = process_get_handle(proc, args->handle);
            if (!h) {
                KPANIC("Failed to find handle");
                return 0;
            }

            // io_device_t * d = h->device;

            // return d->read_fn(d->data, args->buff, args->count, args->pos);
        } break;

        case SYS_CALL_IO_WRITE: {
            struct _args {
                int          handle;
                const char * buff;
                size_t       count;
                size_t       pos;
            } * args = (struct _args *)args_data;

            if (args->handle < 0 || !args->buff || !args->count) {
                return 0;
            }

            handle_t * h = process_get_handle(proc, args->handle);
            if (!h) {
                KPANIC("Failed to find handle");
                return 0;
            }

            io_device_t * d = h->device;

            return d->write_fn(d->device_data, args->buff, args->count, args->pos);
        } break;

        case SYS_CALL_IO_SIZE: {
            struct _args {
                int          handle;
                const char * buff;
                size_t       count;
                size_t       pos;
            } * args = (struct _args *)args_data;

            if (args->handle < 0 || !args->buff || !args->count) {
                return 0;
            }

            handle_t * h = process_get_handle(proc, args->handle);
            if (!h) {
                KPANIC("Failed to find handle");
                return 0;
            }

            io_device_t * d = h->device;

            return d->size_fn(d->device_data);
        } break;
    }

    return 0;
}

// static handle_t * get_free_handle(process_t * proc) {
//     arr_t * io_handles = &proc->io_handles;

//     for (size_t i = 0; i < arr_size(io_handles); i++) {
//         handle_t * handle = arr_at(io_handles, i);

//         if (handle->type == HANDLE_TYPE_FREE) {
//             return handle;
//         }
//     }

//     handle_t new_handle;
//     new_handle.id   = arr_size(io_handles) + 1; // index at 1
//     new_handle.type = HANDLE_TYPE_FREE;

//     if (arr_insert(io_handles, arr_size(io_handles), &new_handle)) {
//         return 0;
//     }

//     return arr_at(io_handles, arr_size(io_handles) - 1);
// }
