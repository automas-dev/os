#ifndef KERNEL_LOGS_H
#define KERNEL_LOGS_H

#include <stdarg.h>
#include <stddef.h>

void kernel_log(const char * fmt, ...);
void kernel_service_log(const char * service, const char * fmt, ...);

#endif // KERNEL_LOGS_H
