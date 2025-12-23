#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <stdarg.h>
#include <stddef.h>

enum KERNEL_LOG_LEVEL {
    KERNEL_LOG_LEVEL_TRACE = 0,
    KERNEL_LOG_LEVEL_DEBUG,
    KERNEL_LOG_LEVEL_INFO,
    KERNEL_LOG_LEVEL_WARNING,
    KERNEL_LOG_LEVEL_ERROR,
};

#define VA_ARGS(...)           , ##__VA_ARGS__
#define KLOG_TRACE(FMT, ...)   kernel_log(KERNEL_LOG_LEVEL_TRACE, 0, (FMT)VA_ARGS(__VA_ARGS__))
#define KLOG_DEBUG(FMT, ...)   kernel_log(KERNEL_LOG_LEVEL_DEBUG, 0, (FMT)VA_ARGS(__VA_ARGS__))
#define KLOG_INFO(FMT, ...)    kernel_log(KERNEL_LOG_LEVEL_INFO, 0, (FMT)VA_ARGS(__VA_ARGS__))
#define KLOG_WARNING(FMT, ...) kernel_log(KERNEL_LOG_LEVEL_WARNING, 0, (FMT)VA_ARGS(__VA_ARGS__))
#define KLOG_ERROR(FMT, ...)   kernel_log(KERNEL_LOG_LEVEL_ERROR, 0, (FMT)VA_ARGS(__VA_ARGS__))

#define KLOGS_TRACE(SERVICE, FMT, ...)   kernel_log(KERNEL_LOG_LEVEL_TRACE, (SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#define KLOGS_DEBUG(SERVICE, FMT, ...)   kernel_log(KERNEL_LOG_LEVEL_DEBUG, (SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#define KLOGS_INFO(SERVICE, FMT, ...)    kernel_log(KERNEL_LOG_LEVEL_INFO, (SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#define KLOGS_WARNING(SERVICE, FMT, ...) kernel_log(KERNEL_LOG_LEVEL_WARNING, (SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#define KLOGS_ERROR(SERVICE, FMT, ...)   kernel_log(KERNEL_LOG_LEVEL_ERROR, (SERVICE), (FMT)VA_ARGS(__VA_ARGS__))

void kernel_log_init();

void kernel_log_enable();
void kernel_log_disable();

void kernel_log_time_enable();
void kernel_log_time_disable();

void kernel_log_set_level(int level);

void kernel_log(int level, const char * service, const char * fmt, ...);

#endif // KERNEL_LOG_H
