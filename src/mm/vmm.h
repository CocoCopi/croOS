/* croOS vmm.h — Virtual Memory Manager (x86 paging) */
#ifndef _VMM_H
#define _VMM_H

#include "kernel/types.h"

#define PAGE_PRESENT  0x01
#define PAGE_WRITE    0x02
#define PAGE_USER     0x04
#define PAGE_DIRTY    0x40

typedef struct {
    uint32_t entries[1024];
} __aligned(4096) page_dir_t;

typedef struct {
    uint32_t entries[1024];
} __aligned(4096) page_tab_t;

void     vmm_init(void);
void     vmm_map_page(page_dir_t *dir, uint32_t virt, uint32_t phys, uint32_t flags);
void     vmm_unmap_page(page_dir_t *dir, uint32_t virt);
page_dir_t *vmm_create_dir(void);
void     vmm_switch_dir(page_dir_t *dir);
uint32_t vmm_get_phys(page_dir_t *dir, uint32_t virt);

#endif
