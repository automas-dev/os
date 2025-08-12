#include "libk/sys_call.h"

#include <stdint.h>

#include "libk/defs.h"

#define PTR2UINT(PTR)   ((uint32_t)(PTR))
#define UINT2PTR(UINT)  ((void *)(UINT))
#define LUINT2PTR(UINT) UINT2PTR((uint32_t)(UINT))

extern int            send_call(uint32_t int_no, ...);
extern NO_RETURN void send_call_noret(uint32_t int_no, ...);

int _sys_io_open(const char * path, const char * mode) {
    return send_call(SYS_INT_IO_OPEN, path, mode);
}

int _sys_io_close(int handle) {
    return send_call(SYS_INT_IO_CLOSE, handle);
}

int _sys_io_read(int handle, char * buff, size_t count) {
    return send_call(SYS_INT_IO_READ, handle, buff, count);
}

int _sys_io_write(int handle, const char * buff, size_t count) {
    return send_call(SYS_INT_IO_WRITE, handle, buff, count);
}

int _sys_io_seek(int handle, int pos, int seek) {
    return send_call(SYS_INT_IO_SEEK, handle, pos, seek);
}

int _sys_io_tell(int handle) {
    return send_call(SYS_INT_IO_TELL, handle);
}

void * _sys_mem_malloc(size_t size) {
    return UINT2PTR(send_call(SYS_INT_MEM_MALLOC, size));
}

void * _sys_mem_realloc(void * ptr, size_t size) {
    return UINT2PTR(send_call(SYS_INT_MEM_REALLOC, ptr, size));
}

void _sys_mem_free(void * ptr) {
    send_call(SYS_INT_MEM_FREE, ptr);
}

void _sys_proc_exit(uint8_t code) {
    _sys_puts("libk: Proc exit\n");
    send_call_noret(SYS_INT_PROC_EXIT, code);
}

void _sys_proc_abort(uint8_t code, const char * msg) {
    _sys_puts("libk: Proc abort\n");
    send_call_noret(SYS_INT_PROC_ABORT, code, msg);
}

void _sys_proc_panic(const char * msg, const char * file, unsigned int line) {
    _sys_puts("libk: Proc panic\n");
    send_call_noret(SYS_INT_PROC_PANIC, msg, file, line);
}

int _sys_proc_getpid(void) {
    return send_call(SYS_INT_PROC_GETPID);
}

void _sys_register_signals(void * callback) {
    send_call(SYS_INT_PROC_REG_SIG, callback);
}

void _sys_queue_event(ebus_event_t * event) {
    send_call(SYS_INT_PROC_QUEUE_EVENT, event);
}

int _sys_yield(int filter, ebus_event_t * event_out) {
    return send_call(SYS_INT_PROC_YIELD, filter, event_out);
}

int _sys_proc_exec(const char * filename, int argc, char ** argv) {
    return send_call(SYS_INT_PROC_EXEC, filename, argc, argv);
}

size_t _sys_putc(char c) {
    return send_call(SYS_INT_STDIO_PUTC, c);
}

size_t _sys_puts(const char * str) {
    return send_call(SYS_INT_STDIO_PUTS, str);
}

file_t _sys_io_file_open(const char * path, const char * mode) {
    return send_call(SYS_INT_IO_FILE_OPEN, path, mode);
}

void _sys_io_file_close(file_t fp) {
    send_call(SYS_INT_IO_FILE_CLOSE, fp);
}

size_t _sys_io_file_read(file_t fp, size_t size, size_t count, void * buff) {
    return send_call(SYS_INT_IO_FILE_READ, fp, size, count, buff);
}

size_t _sys_io_file_write(file_t fp, size_t size, size_t count, const void * buff) {
    return send_call(SYS_INT_IO_FILE_WRITE, fp, size, count, buff);
}

int _sys_io_file_seek(file_t fp, int offset, int origin) {
    return send_call(SYS_INT_IO_FILE_SEEK, fp, offset, origin);
}

int _sys_io_file_tell(file_t fp) {
    return send_call(SYS_INT_IO_FILE_TELL, fp);
}

dir_t _sys_io_dir_open(const char * path) {
    return send_call(SYS_INT_IO_DIR_OPEN, path);
}

void _sys_io_dir_close(dir_t dp) {
    send_call(SYS_INT_IO_DIR_CLOSE, dp);
}

int _sys_io_dir_read(dir_t dp, void * dir_entry) {
    return send_call(SYS_INT_IO_DIR_READ, dp, dir_entry);
}

int _sys_io_dir_seek(dir_t dp, int offset, int origin) {
    return send_call(SYS_INT_IO_DIR_SEEK, dp, offset, origin);
}

int _sys_io_dir_tell(dir_t dp) {
    return send_call(SYS_INT_IO_DIR_TELL, dp);
}
