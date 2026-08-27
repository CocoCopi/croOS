/* croOS gdt.h — Global Descriptor Table (x86 protected mode) */
#ifndef _GDT_H
#define _GDT_H

#include "kernel/types.h"

#define GDT_ENTRIES 6

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __packed gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __packed gdt_ptr_t;

void gdt_init(void);

#endif
