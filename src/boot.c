/* croOS — boot stub
 * Multiboot header + GDT setup, in pure C (no NASM required).
 * The kernel entry point calls kmain() which is linked from the
 * Corros-written shell (compiled to C via --compile).
 */
#include <stdint.h>

#define MULTIBOOT_MAGIC  0x1BADB002
#define MULTIBOOT_FLAGS  0x00010000  /* page-align modules, memory info */
#define CHECKSUM         -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

/* Multiboot header — must be in the first 8 KiB of the binary. */
__attribute__((section(".multiboot"), used))
const uint32_t multiboot_header[3] = {
    MULTIBOOT_MAGIC,
    MULTIBOOT_FLAGS,
    CHECKSUM
};

/* Initial 8 KiB stack (in BSS). */
static uint8_t kernel_stack[8192] __attribute__((aligned(16)));

/* ---- GDT ------------------------------------------------------------------ */

typedef struct {
    uint16_t limit_lo;
    uint16_t base_lo;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  flags;
    uint8_t  base_hi;
} __attribute__((packed)) GdtEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) GdtPtr;

static GdtEntry gdt[3];
static GdtPtr   gdt_ptr;

static void gdt_set_gate(int idx, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t flags)
{
    gdt[idx].base_lo  = base & 0xFFFF;
    gdt[idx].base_mid = (base >> 16) & 0xFF;
    gdt[idx].base_hi  = (base >> 24) & 0xFF;
    gdt[idx].limit_lo = limit & 0xFFFF;
    gdt[idx].access   = access;
    gdt[idx].flags    = flags;
}

static void gdt_install(void)
{
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;
    gdt_set_gate(0, 0, 0,    0, 0);              /* null   */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);  /* kernel code */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);  /* kernel data */
    /* Reload GDT via lgdt + far jump (using inline asm). */
    __asm__ volatile (
        "lgdt (%0)\n\t"
        "ljmp $0x08, $.Lgdt_flush\n\t"
        ".Lgdt_flush:\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "r"(&gdt_ptr) : "eax"
    );
}

/* ---- IDT stub (empty, just enough to not crash) -------------------------- */

typedef struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed)) IdtEntry;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) IdtPtr;

static IdtEntry idt[256];
static IdtPtr   idt_ptr;

static void idt_set_gate(int idx, uint32_t base, uint16_t sel,
                         uint8_t flags)
{
    idt[idx].base_lo = base & 0xFFFF;
    idt[idx].base_hi = (base >> 16) & 0xFFFF;
    idt[idx].sel     = sel;
    idt[idx].always0 = 0;
    idt[idx].flags   = flags;
}

/* Simple ISR stub that just does iret. */
__attribute__((used))
static void isr_stub(void)
{
    __asm__ volatile ("iret");
}

static void idt_install(void)
{
    uint32_t stub = (uint32_t)isr_stub;
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint32_t)&idt;
    for (int i = 0; i < 256; i++)
        idt_set_gate(i, stub, 0x08, 0x8E);
    __asm__ volatile ("lidt (%0)" : : "r"(&idt_ptr));
}

/* ---- Entry point ---------------------------------------------------------- */

/* The Corros-generated code declares this in its own compilation unit.
 * We declare it here to be safe. */
extern void kmain(void);

void _start(void)
{
    gdt_install();
    idt_install();
    kmain();
    /* If kmain returns, halt. */
    for (;;) __asm__ volatile ("hlt");
}
