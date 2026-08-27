/* croOS vmm.c — Virtual Memory Manager
 * x86 two-level page tables: Page Directory → Page Table → Physical frame
 * Identity maps the first 4MB (kernel), allows dynamic mapping for user space. */

#include "kernel/types.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "string.h"

static page_dir_t *kernel_dir = NULL;

/* Assembly: load page directory and enable paging */
extern void vmm_flush_tlb(void);
extern void vmm_load_directory(uint32_t dir_phys);

/* Helpers */
static inline uint32_t page_dir_index(uint32_t virt) { return (virt >> 22) & 0x3FF; }
static inline uint32_t page_tab_index(uint32_t virt) { return (virt >> 12) & 0x3FF; }

static page_tab_t *get_or_create_table(page_dir_t *dir, uint32_t dir_idx, uint32_t flags) {
    if (dir->entries[dir_idx] & PAGE_PRESENT) {
        return (page_tab_t*)(dir->entries[dir_idx] & 0xFFFFF000);
    }
    void *frame = pmm_alloc_page();
    if (!frame) return NULL;
    memset(frame, 0, PAGE_SIZE);
    dir->entries[dir_idx] = (uint32_t)frame | flags;
    return (page_tab_t*)frame;
}

void vmm_init(void) {
    /* Allocate kernel page directory */
    kernel_dir = (page_dir_t*)pmm_alloc_page();
    memset(kernel_dir, 0, PAGE_SIZE);

    /* Identity map first 4MB (kernel + video memory + BIOS) */
    for (uint32_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        vmm_map_page(kernel_dir, addr, addr, PAGE_PRESENT | PAGE_WRITE);
    }

    /* Enable paging */
    vmm_load_directory((uint32_t)kernel_dir);

    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  /* PG bit */
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void vmm_map_page(page_dir_t *dir, uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t di = page_dir_index(virt);
    uint32_t ti = page_tab_index(virt);

    page_tab_t *table = get_or_create_table(dir, di, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    if (!table) return;

    table->entries[ti] = (phys & 0xFFFFF000) | (flags & 0xFFF);
    vmm_flush_tlb();
}

void vmm_unmap_page(page_dir_t *dir, uint32_t virt) {
    uint32_t di = page_dir_index(virt);
    uint32_t ti = page_tab_index(virt);

    if (!(dir->entries[di] & PAGE_PRESENT)) return;
    page_tab_t *table = (page_tab_t*)(dir->entries[di] & 0xFFFFF000);
    table->entries[ti] = 0;
    vmm_flush_tlb();
}

page_dir_t *vmm_create_dir(void) {
    page_dir_t *dir = (page_dir_t*)pmm_alloc_page();
    if (!dir) return NULL;
    memset(dir, 0, PAGE_SIZE);

    /* Share kernel pages (first 4MB) with new directory */
    for (uint32_t i = 0; i < 1024; i++) {
        if (kernel_dir->entries[i] & PAGE_PRESENT) {
            dir->entries[i] = kernel_dir->entries[i];
        }
    }
    return dir;
}

void vmm_switch_dir(page_dir_t *dir) {
    vmm_load_directory((uint32_t)dir);
}

uint32_t vmm_get_phys(page_dir_t *dir, uint32_t virt) {
    uint32_t di = page_dir_index(virt);
    uint32_t ti = page_tab_index(virt);

    if (!(dir->entries[di] & PAGE_PRESENT)) return 0;
    page_tab_t *table = (page_tab_t*)(dir->entries[di] & 0xFFFFF000);
    if (!(table->entries[ti] & PAGE_PRESENT)) return 0;
    return table->entries[ti] & 0xFFFFF000;
}
