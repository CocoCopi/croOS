/* croOS pmm.c — Physical Memory Manager
 * Simple bitmap allocator: each bit represents one 4KB page frame.
 * 0 = free, 1 = used. Memory above 4GB is ignored (32-bit kernel). */

#include "kernel/types.h"
#include "mm/pmm.h"
#include "string.h"

#define BITMAP_SIZE (128 * 1024 / 8)  /* support up to 128 MB */

static uint8_t bitmap[BITMAP_SIZE];
static uint32_t total_pages;
static uint32_t used_pages;

/* Bitmap helpers */
static void bitmap_set(uint32_t page)   { bitmap[page / 8] |=  (1 << (page % 8)); }
static void bitmap_clear(uint32_t page) { bitmap[page / 8] &= ~(1 << (page % 8)); }
static int  bitmap_test(uint32_t page)  { return bitmap[page / 8] & (1 << (page % 8)); }

void pmm_init(uint32_t mem_size_bytes) {
    total_pages = mem_size_bytes / PAGE_SIZE;
    if (total_pages > BITMAP_SIZE * 8) total_pages = BITMAP_SIZE * 8;
    used_pages = 0;

    /* Mark all as used first */
    memset(bitmap, 0xFF, BITMAP_SIZE);

    /* Mark pages below 1MB as used (BIOS, VGA, kernel) */
    for (uint32_t i = 0; i < 256; i++) bitmap_set(i);  /* 0 - 1MB */

    /* Mark pages above kernel end as free */
    extern uint32_t _kernel_end;
    uint32_t kernel_end = (uint32_t)&_kernel_end;
    uint32_t start_page = (kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t i = start_page; i < total_pages; i++) {
        bitmap_clear(i);
    }

    used_pages = start_page;
}

void *pmm_alloc_page(void) {
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_pages++;
            return (void*)(i * PAGE_SIZE);
        }
    }
    return NULL;  /* out of memory */
}

void pmm_free_page(void *addr) {
    uint32_t page = (uint32_t)addr / PAGE_SIZE;
    if (page < total_pages && bitmap_test(page)) {
        bitmap_clear(page);
        used_pages--;
    }
}

uint32_t pmm_get_free_pages(void)  { return total_pages - used_pages; }
uint32_t pmm_get_total_pages(void) { return total_pages; }
