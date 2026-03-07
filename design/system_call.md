# WIP - System Calls

- [x] How system calls are defined (int no.)
- [x] How system calls are sent (wrapper)
- [ ] How system calls are receive (callback)
- [ ] System call wrapper
- [ ] Receiving args
- [ ] Returning value
- [ ] When virtual memory changes, args are invalid? Or not because it's the kernel stack

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

- handled by callback
- registered using system_call_register

# OLD

System calls are performed through interrupt 48 (`int 0x30`). The interrupt
takes a `uint32_t` id and some number of arguments.

Each system call is defined by a function in libk which wraps the `send_call`
and `send_call_noret` functions.

```c
extern int            send_call(uint32_t int_no, ...);
extern NO_RETURN void send_call_noret(uint32_t int_no, ...);
```

<!-- Each interrupt takes up to 3 arguments and returns a `uint32_t`. -->

TODO - max 256 callbacks per family, family is a 24 bit number
TODO - register callbacks
TODO - callback signature
TODO - accessing args in callback
TODO - args not available after page swap, need to copy anything not on the stack into kernel memory

## System Calls

These are calls from the process to the kernel

> [!WARNING] This section is out of date
> See [src/libk/include/libk/defs.h](../src/libk/include/libk/defs.h) for the
> current implementation.

| ID   | Family          |
| ---- | --------------- |
| 0x01 | I/O             |
| 0x02 | Memory          |
| 0x03 | Process Control |
| 0x10 | Tmp Std I/O     |

An interrupt id is an 8 bit family + an 8 bit id.

| Family          | ID     | Name                                                                      |
| --------------- | ------ | ------------------------------------------------------------------------- |
| I/O             | 0x00010000 | open                                                                      |
|                 | 0x00010001 | close                                                                     |
|                 | 0x00010002 | read                                                                      |
|                 | 0x00010003 | write                                                                     |
|                 | 0x00010004 | seek                                                                      |
|                 | 0x00010005 | tell                                                                      |
| Memory          | 0x00020000 | `void * malloc(size_t size)`                                              |
|                 | 0x00020001 | `void * realloc(void * ptr, size_t size)`                                 |
|                 | 0x00020002 | `void free(void * ptr)`                                                   |
| Process Control | 0x00030000 | `void exit(uint8_t code)`                                                 |
|                 | 0x00030001 | `void abort(uint8_t code, const char * msg)`                              |
|                 | 0x00030002 | `void panic(const char * msg, const char * file, unsigned int line)`      |
|                 | 0x00030003 | `int register_signals(void * callback)`                                   |
|                 | 0x00030004 | `int getpid()`                                                            |
| Tmp Std I/O     | 0x00100000 | `size_t putc(char c)`                                                     |
|                 | 0x00100001 | `size_t puts(const char * str)`                                           |
|                 | 0x00100002 | `size_t vprintf(const char * fmt, va_list params)`                        |
| File I/O        | 0x00110001 | `file_t file_open(const char * path, const char * mode)`                  |
|                 | 0x00110002 | `void file_close(file_t)`                                                 |
|                 | 0x00110003 | `size_t file_read(file_t, size_t size, size_t count, void * buff)`        |
|                 | 0x00110004 | `size_t file_write(file_t, size_t size, size_t count, const void * buff)` |
|                 | 0x00110005 | `int file_seek(file_t, int offset, int origin)`                           |
|                 | 0x00110005 | `int file_tell(file_t)`                                                   |
| Dir I/O         | 0x00120001 | `dir_t dir_open(const char * path)`                                       |
|                 | 0x00120002 | `void dir_close(dir_t)`                                                   |
|                 | 0x00120003 | `int dir_read(dir_t, void * dir_entry)`                                   |
|                 | 0x00120005 | `int dir_seek(dir_t, int offset, int origin)`                             |
|                 | 0x00120005 | `int dir_tell(dir_t)`                                                     |

## System Calls 2.0

io
- open handle
- close handle
- read handle
- write handle
- handle size? (maybe part of open)
- stat? size? (can't be seek or tell, those are in libc)


# System Signals

These are callbacks from the kernel to the process.

The `register_signals` call will hook a function in libc to receive all signals.
It will then store all registered callbacks of the process.

TODO - keyboard event
