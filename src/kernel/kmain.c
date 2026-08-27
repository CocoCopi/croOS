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

/* Multiboot constants */
#define MULTIBOOT_MAGIC     0x2BADB002
#define MB_FLAG_MEM         (1 << 0)
#define MB_FLAG_VBE         (1 << 11)
#define MB_FLAG_FB          (1 << 12)

/*
 * Correct multiboot info structure layout (i386 spec).
 * All offsets verified against the Multiboot Specification v0.6.96.
 */
typedef struct {
    uint32_t flags;                 /* 0   */
    uint32_t mem_lower;             /* 4   (if flags[0]) */
    uint32_t mem_upper;             /* 8   (if flags[0]) */
    uint32_t boot_device;           /* 12  (if flags[1]) */
    uint32_t cmdline;               /* 16  (if flags[2]) */
    uint32_t mods_count;            /* 20  (if flags[3]) */
    uint32_t mods_addr;             /* 24  (if flags[3]) */
    uint32_t syms[4];               /* 28  (if flags[4,5]) */
    uint32_t mmap_length;           /* 44  (if flags[6]) */
    uint32_t mmap_addr;             /* 48  (if flags[6]) */
    uint32_t drives_length;         /* 52  (if flags[7]) */
    uint32_t drives_addr;           /* 56  (if flags[7]) */
    uint32_t config_table;          /* 60  (if flags[8]) */
    uint32_t boot_loader_name;      /* 64  (if flags[9]) */
    uint32_t apm_table;             /* 68  (if flags[10]) */
    uint32_t vbe_control_info;      /* 72  (if flags[11]) */
    uint32_t vbe_mode_info;         /* 76  (if flags[11]) */
    uint16_t vbe_mode;              /* 80  (if flags[11]) */
    uint16_t vbe_interface_seg;     /* 82  (if flags[11]) */
    uint16_t vbe_interface_off;     /* 84  (if flags[11]) */
    uint16_t vbe_interface_len;     /* 86  (if flags[11]) */
    uint64_t framebuffer_addr;      /* 88  (if flags[12]) */
    uint32_t framebuffer_pitch;     /* 96  (if flags[12]) */
    uint32_t framebuffer_width;     /* 100 (if flags[12]) */
    uint32_t framebuffer_height;    /* 104 (if flags[12]) */
    uint8_t  framebuffer_bpp;       /* 108 (if flags[12]) */
    uint8_t  framebuffer_type;      /* 109 (if flags[12]) */
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

    /* Phase 2: CPU tables (interrupts still OFF) */
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

    /* Phase 9: Detect framebuffer and launch display */
    serial_puts("[croOS] Phase 9: Display\n");

    framebuffer_t fb_info = {0};
    uint8_t gui_mode = 0;

    /* Safely check if bootloader provided framebuffer info */
    if (mbi && (mbi->flags & MB_FLAG_FB)) {
        /* Framebuffer info is present */
        uint32_t fb_addr_lo = (uint32_t)(mbi->framebuffer_addr & 0xFFFFFFFF);
        uint32_t fb_pitch = mbi->framebuffer_pitch;
        uint32_t fb_w = mbi->framebuffer_width;
        uint32_t fb_h = mbi->framebuffer_height;
        uint8_t fb_bpp = mbi->framebuffer_bpp;

        serial_puts("[croOS] FB raw: addr=");
        serial_put_hex(fb_addr_lo);
        serial_puts(" pitch=");
        serial_put_dec(fb_pitch);
        serial_puts(" ");
        serial_put_dec(fb_w);
        serial_puts("x");
        serial_put_dec(fb_h);
        serial_puts("@");
        serial_put_dec(fb_bpp);
        serial_puts("\n");

        /* Validate: address must be non-zero, dimensions must be sane */
        if (fb_addr_lo != 0 && fb_w >= 320 && fb_w <= 4096 &&
            fb_h >= 200 && fb_h <= 4096 && fb_bpp >= 16) {
            fb_info.address   = (uint32_t*)fb_addr_lo;
            fb_info.pitch     = fb_pitch;
            fb_info.width     = fb_w;
            fb_info.height    = fb_h;
            fb_info.bpp       = fb_bpp;
            fb_info.available = 1;

            serial_puts("[croOS] Framebuffer validated OK\n");
            fb_init(&fb_info);
            gui_mode = fb_available();
        } else {
            serial_puts("[croOS] FB invalid - using text mode\n");
        }
    } else {
        serial_puts("[croOS] No FB info from bootloader\n");
        serial_puts("[croOS] Trying QEMU VESA fallback...\n");

        /* QEMU fallback: try common VESA framebuffer addresses */
        /* When running with -vga std, QEMU maps the framebuffer
         * at a high physical address. We probe a few known ones. */
        uint32_t probe_addrs[] = { 0xFD000000, 0xFC000000, 0xFE000000, 0 };
        uint32_t probe_sizes[] = { 1024*768*4, 1280*1024*4, 800*600*4, 0 };
        uint32_t probe_w[]     = { 1024, 1280, 800, 0 };
        uint32_t probe_h[]     = { 768, 1024, 600, 0 };

        for (int i = 0; probe_addrs[i] != 0; i++) {
            /* Check if memory is accessible (not all zeros, not page fault) */
            volatile uint32_t *test = (volatile uint32_t*)probe_addrs[i];
            /* Just reading from a high address might page fault if not mapped.
             * Since we identity-mapped first 4MB only, these addresses (>=4MB)
             * will page fault. That's expected and means no framebuffer there.
             * We can't safely probe without full paging set up.
             * So we skip this and just use text mode. */
            (void)test;
            (void)probe_sizes;
            (void)probe_w;
            (void)probe_h;
            break;
        }
    }

    if (gui_mode) {
        vga_disable_cursor();
        serial_puts("[croOS] GUI MODE - HyperCorros\n");
        serial_puts("[croOS] ================================\n");
    } else {
        serial_puts("[croOS] Text mode (no framebuffer)\n");
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
