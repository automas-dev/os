#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stddef.h>

void init_kmalloc(size_t next_heap_page);

void * kmalloc(size_t size);
void * krealloc(void * ptr, size_t size);
void   kfree(void * ptr);

#endif // KERNEL_MEMORY_H
