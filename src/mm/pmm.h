/* croOS pmm.h — Physical Memory Manager (bitmap-based page allocator) */
#ifndef _PMM_H
#define _PMM_H

#include "kernel/types.h"

#define PAGE_SIZE 4096

void pmm_init(uint32_t mem_size_bytes);
void *pmm_alloc_page(void);
void  pmm_free_page(void *addr);
uint32_t pmm_get_free_pages(void);
uint32_t pmm_get_total_pages(void);

#endif
