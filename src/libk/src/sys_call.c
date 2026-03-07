#include "libk/sys_call.h"

#include <stdint.h>

#include "libk/defs.h"

#define PTR2UINT(PTR)   ((uint32_t)(PTR))
#define UINT2PTR(UINT)  ((void *)(UINT))
#define LUINT2PTR(UINT) UINT2PTR((uint32_t)(UINT))

extern int            send_call(uint32_t call_id, ...);
extern NO_RETURN void send_call_noret(uint32_t call_id, ...);

int _sys_io_open(const char * path, const char * mode) {
    return send_call(SYS_CALL_IO_OPEN, path, mode);
}

int _sys_io_close(int handle) {
    return send_call(SYS_CALL_IO_CLOSE, handle);
}

int _sys_io_read(int handle, char * buff, size_t count, size_t pos) {
    return send_call(SYS_CALL_IO_READ, handle, buff, count, pos);
}

int _sys_io_write(int handle, const char * buff, size_t count, size_t pos) {
    return send_call(SYS_CALL_IO_WRITE, handle, buff, count, pos);
}

int _sys_io_size(int handle) {
    return send_call(SYS_CALL_IO_SIZE, handle);
}

void * _sys_mem_malloc(size_t size) {
    return UINT2PTR(send_call(SYS_CALL_MEM_MALLOC, size));
}

void * _sys_mem_realloc(void * ptr, size_t size) {
    return UINT2PTR(send_call(SYS_CALL_MEM_REALLOC, ptr, size));
}

void _sys_mem_free(void * ptr) {
    send_call(SYS_CALL_MEM_FREE, ptr);
}

void _sys_proc_exit(uint8_t code) {
    // _sys_puts("libk: Proc exit\n");
    send_call_noret(SYS_CALL_PROC_EXIT, code);
}

void _sys_proc_abort(uint8_t code, const char * msg) {
    // _sys_puts("libk: Proc abort\n");
    send_call_noret(SYS_CALL_PROC_ABORT, code, msg);
}

void _sys_proc_panic(const char * msg, const char * file, unsigned int line) {
    // _sys_puts("libk: Proc panic\n");
    send_call_noret(SYS_CALL_PROC_PANIC, msg, file, line);
}

int _sys_proc_getpid(void) {
    return send_call(SYS_CALL_PROC_GETPID);
}

void _sys_register_signals(void * callback) {
    send_call(SYS_CALL_PROC_REG_SIG, callback);
}

void _sys_queue_event(ebus_event_t * event) {
    send_call(SYS_CALL_PROC_QUEUE_EVENT, event);
}

int _sys_yield(int filter, ebus_event_t * event_out) {
    return send_call(SYS_CALL_PROC_YIELD, filter, event_out);
}

int _sys_proc_exec(const char * filename, int argc, char ** argv) {
    return send_call(SYS_CALL_PROC_EXEC, filename, argc, argv);
}

int _sys_proc_set_foreground(int pid) {
    return send_call(SYS_CALL_PROC_SET_FOREGROUND, pid);
}
