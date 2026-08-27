/* croOS idt.h — Interrupt Descriptor Table */
#ifndef _IDT_H
#define _IDT_H

#include "kernel/types.h"

#define IDT_ENTRIES 256

typedef struct {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __packed idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __packed idt_ptr_t;

typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __packed regs_t;

typedef void (*isr_handler_t)(regs_t*);

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void isr_install_handler(int n, isr_handler_t handler);
void isr_uninstall_handler(int n);

#endif
