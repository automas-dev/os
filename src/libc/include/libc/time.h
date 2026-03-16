#ifndef LIBC_TIME_H
#define LIBC_TIME_H

#include <stddef.h>

size_t time(void);
void   sleep(size_t ms);
void   usleep(size_t us);

#endif // LIBC_TIME_H
