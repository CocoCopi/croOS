/* croOS gdt.c — Global Descriptor Table */
#include "kernel/gdt.h"

static gdt_entry_t gdt[GDT_ENTRIES];
static gdt_ptr_t   gp;

extern void gdt_flush(uint32_t);

static void gdt_set_entry(int idx, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t gran) {
    gdt[idx].base_low    = base & 0xFFFF;
    gdt[idx].base_mid    = (base >> 16) & 0xFF;
    gdt[idx].base_high   = (base >> 24) & 0xFF;
    gdt[idx].limit_low   = limit & 0xFFFF;
    gdt[idx].granularity  = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[idx].access      = access;
}

void gdt_init(void) {
    gp.limit = sizeof(gdt_entry_t) * GDT_ENTRIES - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0,       0,    0);    /* null  */
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); /* kernel code */
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF); /* kernel data */
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); /* user code */
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); /* user data */
    gdt_set_entry(5, 0, 0,       0,    0);    /* reserved TSS */

    gdt_flush((uint32_t)&gp);
}
