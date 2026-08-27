/* croOS kmalloc.h — Kernel heap allocator (first-fit with free list) */
#ifndef _KMALLOC_H
#define _KMALLOC_H

#include "kernel/types.h"

#define KHEAP_START  0x00400000
#define KHEAP_END    0x02000000  /* 32MB heap */

void  kmalloc_init(void);
void *kmalloc(uint32_t size);
void *kmalloc_aligned(uint32_t size);
void *kcalloc(uint32_t count, uint32_t size);
void *krealloc(void *ptr, uint32_t size);
void  kfree(void *ptr);

#endif
