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
int _sys_io_read(int handle, char * buff, size_t count);
int _sys_io_write(int handle, const char * buff, size_t count);
int _sys_io_seek(int handle, int pos, int seek);
int _sys_io_tell(int handle);

void * _sys_mem_malloc(size_t size);
void * _sys_mem_realloc(void * ptr, size_t size);
void   _sys_mem_free(void * ptr);

NO_RETURN void _sys_proc_exit(uint8_t code);
NO_RETURN void _sys_proc_abort(uint8_t code, const char * msg);
NO_RETURN void _sys_proc_panic(const char * msg, const char * file, unsigned int line);

int _sys_proc_exec(const char * filename, int argc, char ** argv);

int _sys_proc_getpid(void);

void _sys_register_signals(void * callback);
void _sys_queue_event(ebus_event_t * event);
int  _sys_yield(int filter, ebus_event_t * event_out);

size_t _sys_putc(char c);
size_t _sys_puts(const char * str);

typedef int file_t;

file_t _sys_io_file_open(const char * path, const char * mode);
void   _sys_io_file_close(file_t fp);
size_t _sys_io_file_read(file_t fp, size_t size, size_t count, void * buff);
size_t _sys_io_file_write(file_t fp, size_t size, size_t count, const void * buff);
int    _sys_io_file_seek(file_t fp, int offset, int origin);
int    _sys_io_file_tell(file_t fp);

typedef int dir_t;

dir_t _sys_io_dir_open(const char * path);
void  _sys_io_dir_close(dir_t);
int   _sys_io_dir_read(dir_t, void * dir_entry);
int   _sys_io_dir_seek(dir_t, int offset, int origin);
int   _sys_io_dir_tell(dir_t);

#endif // LIBK_SYS_CALL_H
