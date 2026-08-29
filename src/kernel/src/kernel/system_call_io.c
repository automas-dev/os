#define KLOG_SERVICE "KERNEL/SYSTEM_CALL_IO"

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

            if (!args->path) {
                KLOG_WARNING("Open called with null path");
                return 0;
            }
            if (!args->mode) {
                KLOG_WARNING("Open called with null mode");
                return 0;
            }
            if (!*args->path) {
                KLOG_WARNING("Open called with empty path");
                return 0;
            }
            if (!*args->mode) {
                KLOG_WARNING("Open called with empty mode");
                return 0;
            }

            io_device_t * d = device_fs_file_open(args->path, args->mode);
            if (!d) {
                // already logged as warning in device_fs_file_open
                // TODO should this be debug or info or nothing?
                KLOG_DEBUG("Failed to open fs file for %s in mode %s", args->path, args->mode);
                return 0;
            }

            KLOG_DEBUG("Process %u opens file %s in mode %s", proc->pid, args->path, args->mode);

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

            if (args->handle < 0) {
                KLOG_WARNING("Process %u tried to read from invalid handle", proc->pid);
                return 0;
            }
            if (!args->buff) {
                KLOG_WARNING("Process %u tried to read into null buffer", proc->pid);
                return 0;
            }
            if (!args->count) {
                KLOG_WARNING("Process %u read with 0 count", proc->pid);
                return 0;
            }

            // TODO get stdin handle and use device read
            // TODO add buffer to handle_t

            if (args->handle == 0) {
                KLOG_TRACE("Process %u reading %u characters from stdin", proc->pid, args->count);

                size_t written = 0;
                size_t count   = args->count; // idk if this needs to be copied or can be edited in place

                while (count) {
                    size_t available = io_buffer_length(proc->io_buffer);
                    // Use available as the number to read, limit to count if greater than
                    if (available > count) {
                        available = count;
                    }

                    for (size_t i = 0; i < available; i++) {
                        if (io_buffer_pop(proc->io_buffer, &args->buff[written++])) {
                            KLOG_DEBUG("Failed to pop from io buffer for process %u", proc->pid);
                            // TODO should this be -1?
                            return written;
                        }
                    }
                    count -= available;

                    if (count) {
                        // yield
                        proc->next_event.event_id   = 0;
                        proc->filter_event.event_id = EBUS_EVENT_STDIN_READY;
                        proc->state                 = PROCESS_STATE_WAITING;

                        enable_interrupts();
                        kernel_switch_task();
                    }
                }

                proc->filter_event.event_id = 0;
                proc->next_event.event_id   = 0;

                return written;
            }
            else {
                handle_t * h = process_get_handle(proc, args->handle);
                if (!h) {
                    KLOG_DEBUG("Process %u trying to read from unsupported handle %d", proc->pid, args->handle);
                    return 0;
                }

                return io_device_read(h->device, args->buff, args->count, args->pos);
            }
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

            return io_device_write(h->device, args->buff, args->count, args->pos);
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

            return io_device_size(h->device);
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
