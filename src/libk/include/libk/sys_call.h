#ifndef LIBK_SYS_CALL_H
#define LIBK_SYS_CALL_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "ebus.h"

#ifdef TESTING
#define NO_RETURN
#else
#define NO_RETURN _Noreturn
#endif

int _sys_io_open(const char * path, const char * mode);
int _sys_io_close(int handle);
int _sys_io_read(int handle, char * buff, size_t count, size_t pos);
int _sys_io_write(int handle, const char * buff, size_t count, size_t pos);
int _sys_io_size(int handle);

void * _sys_mem_malloc(size_t size);
void * _sys_mem_realloc(void * ptr, size_t size);
void   _sys_mem_free(void * ptr);

NO_RETURN void _sys_proc_exit(int code);
NO_RETURN void _sys_proc_abort(int code, const char * msg);
NO_RETURN void _sys_proc_panic(const char * msg, const char * file, unsigned int line);

int _sys_proc_exec(const char * filename, int argc, char ** argv);

int _sys_proc_getpid(void);

void _sys_register_signals(void * callback);
void _sys_queue_event(ebus_event_t * event);
void _sys_yield(void);
int  _sys_proc_set_foreground(int pid);
int  _sys_proc_wait_pid(int pid, int * exit_status);

int _sys_event_pull(int filter, ebus_event_t * event_out);

size_t _sys_event_time(void);
void   _sys_event_sleep(size_t ms, size_t us);

const char * _sys_kernel_describe();

#endif // LIBK_SYS_CALL_H
