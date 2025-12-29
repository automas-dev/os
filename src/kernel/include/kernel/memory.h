#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

#include <stddef.h>

void kmalloc_init(size_t next_heap_page);

void * kmalloc(size_t size);
void * krealloc(void * ptr, size_t size);
void   kfree(void * ptr);

void * kernel_alloc_page(size_t count);

#endif // KERNEL_MEMORY_H
