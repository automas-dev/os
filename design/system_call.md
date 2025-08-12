# WIP - System Calls

System calls are performed through interrupt 48 (`int 0x30`). The interrupt
takes a `uint16_t` id and some number of arguments.

Each interrupt takes up to 3 arguments and returns a `uint32_t`.

## System Calls

These are calls from the process to the kernel

| ID   | Family          |
| ---- | --------------- |
| 0x01 | I/O             |
| 0x02 | Memory          |
| 0x03 | Process Control |
| 0x10 | Tmp Std I/O     |

An interrupt id is an 8 bit family + an 8 bit id.

| Family          | ID     | Name                                                                      |
| --------------- | ------ | ------------------------------------------------------------------------- |
| I/O             | 0x0100 | open                                                                      |
|                 | 0x0101 | close                                                                     |
|                 | 0x0102 | read                                                                      |
|                 | 0x0103 | write                                                                     |
|                 | 0x0104 | seek                                                                      |
|                 | 0x0105 | tell                                                                      |
| Memory          | 0x0200 | `void * malloc(size_t size)`                                              |
|                 | 0x0201 | `void * realloc(void * ptr, size_t size)`                                 |
|                 | 0x0202 | `void free(void * ptr)`                                                   |
| Process Control | 0x0300 | `void exit(uint8_t code)`                                                 |
|                 | 0x0301 | `void abort(uint8_t code, const char * msg)`                              |
|                 | 0x0302 | `void panic(const char * msg, const char * file, unsigned int line)`      |
|                 | 0x0303 | `int register_signals(void * callback)`                                   |
|                 | 0x0304 | `int getpid()`                                                            |
| Tmp Std I/O     | 0x1000 | `size_t putc(char c)`                                                     |
|                 | 0x1001 | `size_t puts(const char * str)`                                           |
|                 | 0x1002 | `size_t vprintf(const char * fmt, va_list params)`                        |
| File I/O        | 0x1101 | `file_t file_open(const char * path, const char * mode)`                  |
|                 | 0x1102 | `void file_close(file_t)`                                                 |
|                 | 0x1103 | `size_t file_read(file_t, size_t size, size_t count, void * buff)`        |
|                 | 0x1104 | `size_t file_write(file_t, size_t size, size_t count, const void * buff)` |
|                 | 0x1105 | `int file_seek(file_t, int offset, int origin)`                           |
|                 | 0x1105 | `int file_tell(file_t)`                                                   |
| Dir I/O         | 0x1201 | `dir_t dir_open(const char * path)`                                       |
|                 | 0x1202 | `void dir_close(dir_t)`                                                   |
|                 | 0x1203 | `int dir_read(dir_t, void * dir_entry)`                                   |
|                 | 0x1205 | `int dir_seek(dir_t, int offset, int origin)`                             |
|                 | 0x1205 | `int dir_tell(dir_t)`                                                     |

# System Signals

These are callbacks from the kernel to the process.

The `register_signals` call will hook a function in libc to receive all signals.
It will then store all registered callbacks of the process.

TODO - keyboard event
