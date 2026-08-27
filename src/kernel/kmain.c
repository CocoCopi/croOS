/* croOS kmain.c - Kernel entry point
 * Initializes GDT, IDT, memory, drivers, filesystem, networking.
 * Detects framebuffer from Multiboot and launches HyperCorros GUI.
 * Falls back to text-mode shell if no framebuffer available. */

#include "kernel/types.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/process.h"
#include "kernel/compositor.h"
#include "drivers/vga.h"
#include "drivers/framebuffer.h"
#include "drivers/keyboard.h"
#include "drivers/mouse.h"
#include "drivers/timer.h"
#include "drivers/serial.h"
#include "drivers/pci.h"
#include "mm/pmm.h"
#include "mm/kmalloc.h"
#include "mm/vmm.h"
#include "fs/vfs.h"
#include "fs/ramdisk.h"
#include "net/net.h"
#include "sys/syscall.h"
#include "string.h"

/* Multiboot header (v0.6) */
#define MULTIBOOT_MAGIC 0x2BADB002
#define MULTIBOOT_FLAG_FRAMEBUFFER (1 << 2)

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    /* VBE fields */
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    /* Framebuffer fields (flag bit 2) */
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} __packed multiboot_t;

/* Simple shell (text-mode fallback) */
static void kernel_shell(void) {
    char input[256];
    int pos = 0;

    while (1) {
        vga_set_color(VGA_GREEN, VGA_BLACK);
        vga_puts("croOS> ");
        vga_set_color(VGA_WHITE, VGA_BLACK);
        pos = 0;

        while (1) {
            char c = kb_getchar();
            if (c == '\n') { input[pos] = '\0'; vga_putchar('\n'); break; }
            else if (c == '\b') { if (pos > 0) { pos--; vga_putchar('\b'); } }
            else if (c == 3) { vga_puts("^C\n"); pos = 0; break; }
            else { if (pos < 255) { input[pos++] = c; vga_putchar(c); } }
        }
        if (pos == 0) continue;

        if (strcmp(input, "help") == 0) {
            vga_puts("  help, version, mem, clear, ls, cat, echo, reboot\n");
        } else if (strcmp(input, "version") == 0) {
            vga_puts("  croOS 4.0.0 - HyperCorros Desktop\n");
        } else if (strcmp(input, "clear") == 0) {
            vga_clear();
        } else if (strcmp(input, "reboot") == 0) {
            outb(0x92, 0x03); while(1) hlt();
        } else if (strncmp(input, "echo ", 5) == 0) {
            vga_puts("  "); vga_puts(input + 5); vga_putchar('\n');
        } else if (strcmp(input, "mem") == 0) {
            vga_puts("  Free: "); vga_put_dec(pmm_get_free_pages() * 4);
            vga_puts(" KB / "); vga_put_dec(pmm_get_total_pages() * 4); vga_puts(" KB\n");
        } else {
            vga_puts("  Unknown command. Type 'help'.\n");
        }
    }
}

/* Kernel main - called from boot.S */
void kmain(uint32_t magic, multiboot_t *mbi) {
    (void)magic;

    /* Phase 1: Early console */
    serial_init();
    serial_puts("\n[croOS] ================================\n");
    serial_puts("[croOS] croOS 4.0 - HyperCorros Boot\n");
    serial_puts("[croOS] ================================\n");

    vga_init();
    vga_clear();

    vga_set_color(VGA_CYAN, VGA_BLACK);
    vga_puts("  croOS 4.0 - HyperCorros Desktop\n");
    vga_puts("  Loading kernel...\n");
    serial_puts("[croOS] VGA + serial OK\n");

    /* Phase 2: CPU tables */
    serial_puts("[croOS] GDT...\n");
    gdt_init();
    serial_puts("[croOS] IDT...\n");
    idt_init();
    serial_puts("[croOS] CPU tables OK\n");

    /* Phase 3: Memory */
    uint32_t mem_kb = 32768;
    if (mbi && (mbi->flags & 1) && mbi->mem_lower > 0)
        mem_kb = mbi->mem_lower + 1024;
    uint32_t mem_bytes = mem_kb * 1024;
    if (mem_bytes > 128 * 1024 * 1024) mem_bytes = 128 * 1024 * 1024;

    serial_puts("[croOS] PMM...\n");
    pmm_init(mem_bytes);
    serial_puts("[croOS] Heap...\n");
    kmalloc_init();
    serial_puts("[croOS] VMM...\n");
    vmm_init();
    serial_puts("[croOS] Memory OK (");
    serial_put_dec(mem_kb);
    serial_puts(" KB)\n");
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  [OK] Memory initialized\n");

    /* Phase 4: Drivers */
    serial_puts("[croOS] PIT timer...\n");
    timer_init(100);
    serial_puts("[croOS] Keyboard...\n");
    kb_init();
    serial_puts("[croOS] Mouse...\n");
    mouse_init();
    vga_puts("  [OK] Input drivers loaded\n");

    /* Phase 5: Filesystem */
    serial_puts("[croOS] VFS...\n");
    vfs_init();
    vfs_init_ramdisk();
    serial_puts("[croOS] Filesystem OK\n");

    /* Phase 6: Networking */
    serial_puts("[croOS] Network...\n");
    net_init();
    serial_puts("[croOS] Network OK\n");

    /* Phase 7: Syscalls */
    serial_puts("[croOS] Syscalls...\n");
    syscall_init();
    serial_puts("[croOS] Syscalls OK\n");

    /* Phase 8: Processes */
    serial_puts("[croOS] Scheduler...\n");
    process_init();
    serial_puts("[croOS] Scheduler OK\n");

    /* Phase 9: Detect framebuffer */
    serial_puts("[croOS] Phase 9: Display\n");

    framebuffer_t fb_info = {0};
    uint8_t gui_mode = 0;

    if (mbi && (mbi->flags & MULTIBOOT_FLAG_FRAMEBUFFER) && mbi->framebuffer_addr != 0) {
        fb_info.address  = (uint32_t*)(uint32_t)mbi->framebuffer_addr;
        fb_info.pitch    = mbi->framebuffer_pitch;
        fb_info.width    = mbi->framebuffer_width;
        fb_info.height   = mbi->framebuffer_height;
        fb_info.bpp      = mbi->framebuffer_bpp;
        fb_info.available = 1;

        serial_puts("[croOS] Framebuffer: ");
        serial_put_dec(fb_info.width);
        serial_puts("x");
        serial_put_dec(fb_info.height);
        serial_puts(" @ ");
        serial_put_dec(fb_info.bpp);
        serial_puts("bpp, addr=0x");
        serial_put_hex((uint32_t)fb_info.address);
        serial_puts("\n");

        fb_init(&fb_info);
        gui_mode = fb_available();

        if (gui_mode) {
            vga_disable_cursor();
            serial_puts("[croOS] GUI MODE - HyperCorros compositor\n");
        }
    }

    if (!gui_mode) {
        serial_puts("[croOS] No framebuffer - using text mode\n");
        serial_puts("[croOS] ================================\n");
        serial_puts("[croOS] All subsystems ready. Shell.\n");
    }

    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  [OK] All subsystems initialized!\n");
    vga_putchar('\n');

    /* Phase 10: Enable interrupts */
    serial_puts("[croOS] Enabling interrupts...\n");
    sti();
    serial_puts("[croOS] Interrupts enabled!\n");

    /* Phase 11: Launch GUI or shell */
    if (gui_mode) {
        serial_puts("[croOS] Launching HyperCorros compositor...\n");
        compositor_init();
        app_terminal_create();
        compositor_run();
    } else {
        serial_puts("[croOS] Entering text shell...\n");
        kernel_shell();
    }
}
