/* croOS kmalloc.c — Kernel heap allocator
 * First-fit allocator with a singly-linked free list.
 * Blocks are split on allocation and coalesced on free. */

#include "kernel/types.h"
#include "mm/kmalloc.h"
#include "mm/pmm.h"
#include "string.h"

typedef struct block {
    uint32_t size;       /* usable size (excluding header) */
    uint8_t  free;       /* 1 = free, 0 = allocated */
    struct block *next;
} block_t;

static block_t *heap_head = NULL;
static uint32_t heap_current = 0;

static block_t *find_free(uint32_t size) {
    block_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size) return cur;
        cur = cur->next;
    }
    return NULL;
}

static block_t *expand_heap(uint32_t size) {
    uint32_t needed = sizeof(block_t) + size;
    uint32_t pages = (needed + PAGE_SIZE - 1) / PAGE_SIZE;

    block_t *new_block = NULL;
    for (uint32_t i = 0; i < pages; i++) {
        void *page = pmm_alloc_page();
        if (!page) return NULL;
        /* Map page into kernel heap space (identity map for now) */
    }

    new_block = (block_t*)heap_current;
    new_block->size = pages * PAGE_SIZE - sizeof(block_t);
    new_block->free = 1;
    new_block->next = NULL;

    heap_current += pages * PAGE_SIZE;

    /* Append to list */
    if (!heap_head) {
        heap_head = new_block;
    } else {
        block_t *last = heap_head;
        while (last->next) last = last->next;
        last->next = new_block;
    }
    return new_block;
}

void kmalloc_init(void) {
    heap_current = KHEAP_START;
    heap_head = NULL;
}

void *kmalloc(uint32_t size) {
    if (size == 0) return NULL;

    block_t *blk = find_free(size);
    if (!blk) {
        blk = expand_heap(size);
        if (!blk) return NULL;
    }

    /* Split if the block is significantly larger */
    if (blk->size > size + sizeof(block_t) + 16) {
        block_t *split = (block_t*)((uint8_t*)blk + sizeof(block_t) + size);
        split->size = blk->size - size - sizeof(block_t);
        split->free = 1;
        split->next = blk->next;
        blk->next = split;
        blk->size = size;
    }

    blk->free = 0;
    return (void*)((uint8_t*)blk + sizeof(block_t));
}

void *kmalloc_aligned(uint32_t size) {
    uint32_t aligned_size = (size + 15) & ~15;  /* 16-byte align */
    return kmalloc(aligned_size);
}

void *kcalloc(uint32_t count, uint32_t size) {
    uint32_t total = count * size;
    void *ptr = kmalloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *krealloc(void *ptr, uint32_t size) {
    if (!ptr) return kmalloc(size);
    if (size == 0) { kfree(ptr); return NULL; }

    block_t *blk = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    if (blk->size >= size) return ptr;

    void *new_ptr = kmalloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, blk->size);
        kfree(ptr);
    }
    return new_ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;
    block_t *blk = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    blk->free = 1;

    /* Coalesce with next block if free */
    if (blk->next && blk->next->free) {
        blk->size += sizeof(block_t) + blk->next->size;
        blk->next = blk->next->next;
    }
}
