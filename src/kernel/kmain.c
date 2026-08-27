/* croOS kmain.c - Kernel entry point
 * Initializes GDT, IDT, memory, drivers, filesystem, networking.
 * Detects framebuffer from bootloader (0x500) or Multiboot.
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

/* Multiboot constants */
#define MULTIBOOT_MAGIC     0x2BADB002
#define MB_FLAG_MEM         (1 << 0)
#define MB_FLAG_FB          (1 << 12)

/* Correct multiboot info layout (spec v0.6.96) */
typedef struct {
    uint32_t flags;                 /* 0   */
    uint32_t mem_lower;             /* 4   */
    uint32_t mem_upper;             /* 8   */
    uint32_t boot_device;           /* 12  */
    uint32_t cmdline;               /* 16  */
    uint32_t mods_count;            /* 20  */
    uint32_t mods_addr;             /* 24  */
    uint32_t syms[4];               /* 28  */
    uint32_t mmap_length;           /* 44  */
    uint32_t mmap_addr;             /* 48  */
    uint32_t drives_length;         /* 52  */
    uint32_t drives_addr;           /* 56  */
    uint32_t config_table;          /* 60  */
    uint32_t boot_loader_name;      /* 64  */
    uint32_t apm_table;             /* 68  */
    uint32_t vbe_control_info;      /* 72  */
    uint32_t vbe_mode_info;         /* 76  */
    uint16_t vbe_mode;              /* 80  */
    uint16_t vbe_interface_seg;     /* 82  */
    uint16_t vbe_interface_off;     /* 84  */
    uint16_t vbe_interface_len;     /* 86  */
    uint64_t framebuffer_addr;      /* 88  */
    uint32_t framebuffer_pitch;     /* 96  */
    uint32_t framebuffer_width;     /* 100 */
    uint32_t framebuffer_height;    /* 104 */
    uint8_t  framebuffer_bpp;       /* 108 */
    uint8_t  framebuffer_type;      /* 109 */
} __packed multiboot_t;

/* Bootloader framebuffer info at 0x500 (set by boot.asm) */
typedef struct {
    uint32_t addr;      /* 0x500: framebuffer physical address */
    uint32_t pitch;     /* 0x504: bytes per scanline */
    uint32_t width;     /* 0x508: width in pixels */
    uint32_t height;    /* 0x50C: height in pixels */
    uint32_t bpp;       /* 0x510: bits per pixel */
    uint8_t  available; /* 0x514: 1 if framebuffer is valid */
    uint8_t  pad[3];    /* alignment */
} __packed bootloader_fb_t;

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
            vga_puts("  help, version, mem, clear, ls, echo, reboot\n");
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

/* Kernel main - called from boot.S or bootloader */
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
    if (mbi && (mbi->flags & MB_FLAG_MEM) && mbi->mem_lower > 0)
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

    /* Phase 9: Detect framebuffer and launch GUI */
    serial_puts("[croOS] Phase 9: Display\n");

    framebuffer_t fb_info = {0};
    uint8_t gui_mode = 0;

    /* Source 1: Check bootloader framebuffer info at 0x500 */
    bootloader_fb_t *bfb = (bootloader_fb_t*)0x500;
    if (bfb->available && bfb->addr != 0 && bfb->width >= 320) {
        serial_puts("[croOS] Bootloader FB: addr=");
        serial_put_hex(bfb->addr);
        serial_puts(" ");
        serial_put_dec(bfb->width);
        serial_puts("x");
        serial_put_dec(bfb->height);
        serial_puts("@");
        serial_put_dec(bfb->bpp);
        serial_puts("\n");

        fb_info.address   = (uint32_t*)bfb->addr;
        fb_info.pitch     = bfb->pitch;
        fb_info.width     = bfb->width;
        fb_info.height    = bfb->height;
        fb_info.bpp       = bfb->bpp;
        fb_info.available = 1;
        fb_init(&fb_info);
        gui_mode = fb_available();
    }

    /* Source 2: Check multiboot info (for GRUB boot) */
    if (!gui_mode && mbi && (mbi->flags & MB_FLAG_FB)) {
        uint32_t fb_addr = (uint32_t)(mbi->framebuffer_addr & 0xFFFFFFFF);
        if (fb_addr != 0 && mbi->framebuffer_width >= 320) {
            serial_puts("[croOS] Multiboot FB: addr=");
            serial_put_hex(fb_addr);
            serial_puts(" ");
            serial_put_dec(mbi->framebuffer_width);
            serial_puts("x");
            serial_put_dec(mbi->framebuffer_height);
            serial_puts("@");
            serial_put_dec(mbi->framebuffer_bpp);
            serial_puts("\n");

            fb_info.address   = (uint32_t*)fb_addr;
            fb_info.pitch     = mbi->framebuffer_pitch;
            fb_info.width     = mbi->framebuffer_width;
            fb_info.height    = mbi->framebuffer_height;
            fb_info.bpp       = mbi->framebuffer_bpp;
            fb_info.available = 1;
            fb_init(&fb_info);
            gui_mode = fb_available();
        }
    }

    if (gui_mode) {
        vga_disable_cursor();
        serial_puts("[croOS] GUI MODE - HyperCorros Desktop\n");
        serial_puts("[croOS] ================================\n");
    } else {
        serial_puts("[croOS] No framebuffer - text mode\n");
        serial_puts("[croOS] ================================\n");
    }

    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  [OK] All subsystems initialized!\n");
    vga_putchar('\n');

    /* Phase 10: Enable interrupts */
    serial_puts("[croOS] Enabling interrupts...\n");
    sti();
    serial_puts("[croOS] Interrupts enabled!\n");

    /* Phase 11: Launch */
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
