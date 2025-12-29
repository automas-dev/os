#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

#define KPANIC(MSG) kernel_panic((MSG), __FILE__, __LINE__)

#ifdef TESTING
#define NO_RETURN
#else
#define NO_RETURN _Noreturn
#endif

NO_RETURN void halt();

NO_RETURN void kernel_panic(const char * msg, const char * file, unsigned int line);

#endif // KERNEL_PANIC_H
