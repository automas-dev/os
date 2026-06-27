# System Calls

<!-- Table of Contents only links to level 2 headers -->
\[ [Sending System Calls](#sending-system-calls) \]
\[ [Receiving System Calls](#receiving-system-calls) \]
\[ [Example Handler](#example-handler) \]

System calls are the mechanism by which processes communicate with the kernel
both for sending commands and retrieving data. Some examples include file io,
memory management, process management, etc. System calls are initiated by the
process. To handle communication form kernel to process, use
[signals](signals.md).

## Sending System Calls

System calls are sent to the kernel through interrupt 48 (`int 0x30`) which is
sent to IRQ 16. Each system call has a wrapper function in libk which calls one
of two functions, `send_call` or `send_call_noret`. These functions take a
`uint32_t` call id followed by some number va args.

```c
extern int            send_call(uint32_t int_no, ...);
extern NO_RETURN void send_call_noret(uint32_t int_no, ...);
```

> [!TIP]
> Wrapper functions are declared in
> [src/libk/include/libk/sys_call.h](../src/libk/include/libk/sys_call.h) and
> defined in [src/libk/src/sys_call.c](../src/libk/src/sys_call.c).

### Call Id

The system call id is a `uint32_t` where the first 16 bits are the family
followed by 16 bits for the call number.

| Family  | Call    |
| ------- | ------- |
| 16 bits | 16 bits |

> [!TIP]
> System call ids are defined in
> [src/libk/include/libk/defs.h](../src/libk/include/libk/defs.h).

## Receiving System Calls

System calls are received by the kernel through callbacks registered with
`system_call_register`. The callback function receives a `uint32_t` call id,
`void *` pointer to the va_args pushed onto the stack by `send_call` and
`send_call_noret`, and `registers_t *` object with values of all registers.

```c
typedef int (*sys_call_handler_t)(uint32_t call_id, void * args_data, registers_t * regs);
void system_call_register(uint16_t family, sys_call_handler_t handler);
```

### Call Arguments

Arguments are accessible from **args_data** which is a pointer to the `va_args`
in the caller process stack. A struct can be used to decompose the argument
values from this pointer.

```c
struct _args {
    void * ptr;
    size_t count;
} * args = (struct _args *)args_data;
// use args->ptr or args->count to read values
```

> [!WARNING]
> Argument data is stored in the process stack. After changing the page
> directory the values in `arg_data` will be invalid. Copy values to the kernel
> stack or heap before switching to retrain access.

### Return Value

Each call handler can optionally returns a single `int` value to the caller
process by returning a value from the handler function. If no value is returned
to the caller process, the call handler should return 0.

## Example Handler

A typical call handler uses a switch block to select the correct logic based
on the `call_id`.

```c
// Define handler function
int sys_call_proc_cb(uint32_t call_id, void * args_data, registers_t * regs) {
    // Get the caller process
    process_t * proc = get_current_process();

    // Handle call id
    switch (call_id) {
        // Log warning if call id is unknown
        default: {
            KLOG_WARNING("Invalid call id 0x%X", call_id);
            break;
        }

        // Logic for call id
        case SYS_CALL_MEM_MALLOC: {
            // va args from send_call or send_call_noret
            struct _args {
                size_t size;
            } * args = (struct _args *)args_data;
            // Return int to caller process
            return PTR2UINT(memory_alloc(&proc->memory, args->size));
        } break;

        case SYS_CALL_MEM_FREE: {
            struct _args {
                void * ptr;
            } * args = (struct _args *)args_data;
            memory_free(&proc->memory, args->ptr);
            // No value is returned so handler defaults to 0
        } break;
    }

    // Default return value of 0
    return 0;
}
```
