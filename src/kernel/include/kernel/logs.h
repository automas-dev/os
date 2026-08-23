#ifndef KERNEL_LOGS_H
#define KERNEL_LOGS_H

#include <stdarg.h>
#include <stddef.h>

#define KERNEL_LOG_LEVEL_TRACE   0
#define KERNEL_LOG_LEVEL_DEBUG   1
#define KERNEL_LOG_LEVEL_INFO    2
#define KERNEL_LOG_LEVEL_WARNING 3
#define KERNEL_LOG_LEVEL_ERROR   4
#define KERNEL_LOG_LEVEL__LENGTH 5 // Number of log levels, used to bound name lookup

#define STRINGIZE(x)  STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING   STRINGIZE(__LINE__)
#define PREFIX        __BASE_FILE__ ":" LINE_STRING

#ifndef KLOG_SERVICE
#error "KLOG_SERVICE must be defined before including kernel/logs.h"
#endif

#ifndef KLOG_LEVEL
#define KLOG_LEVEL KERNEL_LOG_LEVEL_DEBUG
#endif

#define VA_ARGS(...) , ##__VA_ARGS__
#if KLOG_LEVEL <= KERNEL_LOG_LEVEL_TRACE
#define KLOG_TRACE(FMT, ...) kernel_log(KERNEL_LOG_LEVEL_TRACE, (__FILE__), (__LINE__), (KLOG_SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#else
#define KLOG_TRACE(FMT, ...)
#endif

#if KLOG_LEVEL <= KERNEL_LOG_LEVEL_DEBUG
#define KLOG_DEBUG(FMT, ...) kernel_log(KERNEL_LOG_LEVEL_DEBUG, (__FILE__), (__LINE__), (KLOG_SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#else
#define KLOG_DEBUG(FMT, ...)
#endif

#if KLOG_LEVEL <= KERNEL_LOG_LEVEL_INFO
#define KLOG_INFO(FMT, ...) kernel_log(KERNEL_LOG_LEVEL_INFO, (__FILE__), (__LINE__), (KLOG_SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#else
#define KLOG_INFO(FMT, ...)
#endif

#if KLOG_LEVEL <= KERNEL_LOG_LEVEL_WARNING
#define KLOG_WARNING(FMT, ...) kernel_log(KERNEL_LOG_LEVEL_WARNING, (__FILE__), (__LINE__), (KLOG_SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#else
#define KLOG_WARNING(FMT, ...)
#endif

#if KLOG_LEVEL <= KERNEL_LOG_LEVEL_ERROR
#define KLOG_ERROR(FMT, ...) kernel_log(KERNEL_LOG_LEVEL_ERROR, (__FILE__), (__LINE__), (KLOG_SERVICE), (FMT)VA_ARGS(__VA_ARGS__))
#else
#define KLOG_ERROR(FMT, ...)
#endif

void kernel_log_init();

void kernel_log_enable();
void kernel_log_disable();

void kernel_log_time_enable();
void kernel_log_time_disable();

void kernel_log_set_level(int level);

void kernel_log(int level, const char * file, size_t lineno, const char * service, const char * fmt, ...);

#endif // KERNEL_LOGS_H
