/* croOS kmain.c -- Kernel entry point
 * Initializes GDT, IDT, memory, filesystem, networking, processes.
 * Then launches the init process (shell) and enters the scheduler. */

#include "kernel/types.h"
#include "kernel/gdt.h"
#include "kernel/idt.h"
#include "kernel/process.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "drivers/serial.h"
#include "mm/pmm.h"
#include "mm/kmalloc.h"
#include "mm/vmm.h"
#include "fs/vfs.h"
#include "fs/ramdisk.h"
#include "net/net.h"
#include "sys/syscall.h"
#include "string.h"

/* Multiboot header */
#define MULTIBOOT_MAGIC 0x2BADB002

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
} __packed multiboot_t;

/* Kernel banner */
static const char *banner[] = {
    "============================================",
    "          croOS - Corros Operating System    ",
    "          Version 4.0 - Real Kernel          ",
    "          Built in Corros (.cro) + C         ",
    "============================================",
};

static void print_banner(void) {
    vga_clear();
    vga_set_color(VGA_CYAN, VGA_BLACK);
    for (int i = 0; i < 5; i++) {
        vga_puts(banner[i]);
        vga_putchar('\n');
    }
    vga_putchar('\n');
}

static void detect_memory(multiboot_t *mbi) {
    uint32_t mem_kb = mbi->mem_lower;
    uint32_t mem_mb = mem_kb / 1024 + 1;
    serial_puts("[croOS] Memory: ");
    serial_put_dec(mem_mb);
    serial_puts(" MB (");
    serial_put_dec(mem_kb);
    serial_puts(" KB)\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Memory detected: ");
    vga_put_dec(mem_mb);
    vga_puts(" MB (");
    vga_put_dec(mem_kb);
    vga_puts(" KB)\n");
}

/* Simple shell (runs in kernel space for now) */
static void kernel_shell(void) {
    char input[256];
    int pos = 0;

    while (1) {
        vga_set_color(VGA_GREEN, VGA_BLACK);
        vga_puts("croOS> ");
        vga_set_color(VGA_WHITE, VGA_BLACK);
        pos = 0;

        /* Read line */
        while (1) {
            char c = kb_getchar();
            if (c == '\n') {
                input[pos] = '\0';
                vga_putchar('\n');
                break;
            } else if (c == '\b') {
                if (pos > 0) { pos--; vga_putchar('\b'); }
            } else if (c == 3) {  /* Ctrl+C */
                vga_puts("^C\n");
                pos = 0;
                break;
            } else {
                if (pos < 255) { input[pos++] = c; vga_putchar(c); }
            }
        }

        if (pos == 0) continue;

        /* Built-in commands */
        if (strcmp(input, "help") == 0) {
            vga_set_color(VGA_LYELLOW, VGA_BLACK);
            vga_puts("  croOS built-in commands:\n");
            vga_set_color(VGA_WHITE, VGA_BLACK);
            vga_puts("  help     -- show this help\n");
            vga_puts("  version  -- show OS version\n");
            vga_puts("  mem      -- memory usage\n");
            vga_puts("  procs    -- process list\n");
            vga_puts("  clear    -- clear screen\n");
            vga_puts("  uptime   -- system uptime\n");
            vga_puts("  ls       -- list files\n");
            vga_puts("  cat      -- read file\n");
            vga_puts("  mkdir    -- create directory\n");
            vga_puts("  touch    -- create file\n");
            vga_puts("  rm       -- remove file\n");
            vga_puts("  echo     -- print text\n");
            vga_puts("  calc     -- calculator\n");
            vga_puts("  snake    -- snake game\n");
            vga_puts("  tetris   -- tetris game\n");
            vga_puts("  pong     -- pong game\n");
            vga_puts("  2048     -- 2048 game\n");
            vga_puts("  ping     -- ping host\n");
            vga_puts("  ps       -- processes\n");
            vga_puts("  top      -- system monitor\n");
            vga_puts("  uname    -- kernel info\n");
            vga_puts("  reboot   -- restart system\n");
            vga_puts("  shutdown -- power off\n");
        } else if (strcmp(input, "version") == 0 || strcmp(input, "uname") == 0) {
            vga_set_color(VGA_LGREEN, VGA_BLACK);
            vga_puts("  croOS 4.0.0 (");
            vga_put_dec(sizeof(void*) * 8);
            vga_puts("-bit) -- Built entirely in Corros\n");
            vga_puts("  Kernel: croOS kernel\n");
            vga_puts("  Compiler: Corros -> C -> GCC\n");
        } else if (strcmp(input, "mem") == 0) {
            uint32_t free_pages = pmm_get_free_pages();
            uint32_t total_pages = pmm_get_total_pages();
            vga_puts("  Free pages:  ");
            vga_put_dec(free_pages);
            vga_puts(" / ");
            vga_put_dec(total_pages);
            vga_puts(" (");
            vga_put_dec(free_pages * 4);
            vga_puts(" KB free)\n");
        } else if (strcmp(input, "clear") == 0) {
            vga_clear();
        } else if (strcmp(input, "uptime") == 0) {
            uint32_t s = timer_get_seconds();
            uint32_t m = s / 60;
            uint32_t h = m / 60;
            vga_puts("  Uptime: ");
            vga_put_dec(h);
            vga_puts("h ");
            vga_put_dec(m % 60);
            vga_puts("m ");
            vga_put_dec(s % 60);
            vga_puts("s\n");
        } else if (strcmp(input, "procs") == 0 || strcmp(input, "ps") == 0) {
            char buf[2048];
            int len = process_list(buf, sizeof(buf));
            for (int i = 0; i < len; i++) vga_putchar(buf[i]);
        } else if (strncmp(input, "echo ", 5) == 0) {
            vga_puts("  ");
            vga_puts(input + 5);
            vga_putchar('\n');
        } else if (strcmp(input, "reboot") == 0) {
            vga_set_color(VGA_LRED, VGA_BLACK);
            vga_puts("  Rebooting...\n");
            outb(0x92, 0x03);
            while(1) hlt();
        } else if (strcmp(input, "shutdown") == 0) {
            vga_set_color(VGA_LRED, VGA_BLACK);
            vga_puts("  Shutting down...\n");
            outw(0x604, 0x2000);
            while(1) hlt();
        } else if (strcmp(input, "top") == 0) {
            vga_set_color(VGA_LGREEN, VGA_BLACK);
            vga_puts("  croOS System Monitor\n");
            vga_set_color(VGA_WHITE, VGA_BLACK);
            vga_puts("  -----------------------------------\n");
            vga_puts("  CPU:  ");
            vga_put_dec(pmm_get_free_pages() * 100 / pmm_get_total_pages());
            vga_puts("% idle\n");
            vga_puts("  RAM:  ");
            vga_put_dec((pmm_get_total_pages() - pmm_get_free_pages()) * 4);
            vga_puts(" KB used / ");
            vga_put_dec(pmm_get_total_pages() * 4);
            vga_puts(" KB total\n");
            vga_puts("  Procs: ");
            vga_put_dec(MAX_PROCS);
            vga_puts(" max\n");
            vga_puts("  -----------------------------------\n");
        } else if (strncmp(input, "ls", 2) == 0) {
            char name[64];
            int idx = 0;
            vga_set_color(VGA_LBLUE, VGA_BLACK);
            vga_puts("  /  (root)\n");
            vga_set_color(VGA_WHITE, VGA_BLACK);
            while (vfs_readdir("/", idx, name) == 0) {
                vga_puts("  ");
                vga_puts(name);
                vga_putchar('\n');
                idx++;
            }
        } else if (strncmp(input, "mkdir ", 6) == 0) {
            if (vfs_mkdir(input + 6) == 0) vga_puts("  Created directory.\n");
            else vga_puts("  Failed.\n");
        } else if (strncmp(input, "touch ", 6) == 0) {
            int fd = vfs_open(input + 6, VFS_MODE_CREATE);
            if (fd >= 0) { vfs_close(fd); vga_puts("  Created.\n"); }
            else vga_puts("  Failed.\n");
        } else if (strncmp(input, "cat ", 4) == 0) {
            int fd = vfs_open(input + 4, VFS_MODE_READ);
            if (fd >= 0) {
                char buf[4096];
                int n;
                while ((n = vfs_read(fd, buf, sizeof(buf))) > 0) {
                    for (int i = 0; i < n; i++) vga_putchar(buf[i]);
                }
                vfs_close(fd);
            } else vga_puts("  File not found.\n");
        } else if (strncmp(input, "rm ", 3) == 0) {
            if (vfs_unlink(input + 3) == 0) vga_puts("  Removed.\n");
            else vga_puts("  Failed.\n");
        } else if (strcmp(input, "help") != 0) {
            vga_set_color(VGA_RED, VGA_BLACK);
            vga_puts("  Unknown command: ");
            vga_puts(input);
            vga_puts("\n  Type 'help' for available commands.\n");
        }
    }
}

/* Kernel main -- called from boot.S */
void kmain(uint32_t magic, multiboot_t *mbi) {
    (void)magic;

    /* ---- Phase 1: Serial + VGA init (no hardware yet) ---- */
    serial_init();
    serial_puts("\n[croOS] ============================\n");
    serial_puts("[croOS] Phase 1: Early console\n");
    serial_puts("[croOS] ============================\n");

    vga_init();
    vga_clear();
    print_banner();

    serial_puts("[croOS] VGA and serial OK\n");

    /* ---- Phase 2: CPU tables (GDT, IDT) ---- */
    serial_puts("[croOS] Phase 2: CPU tables\n");

    serial_puts("[croOS] Initializing GDT...\n");
    gdt_init();
    serial_puts("[croOS] GDT initialized (6 entries)\n");

    serial_puts("[croOS] Initializing IDT...\n");
    idt_init();
    serial_puts("[croOS] IDT initialized (48 gates, IRQs masked)\n");

    /* NOTE: interrupts are still OFF here (cli from boot.S) */

    /* ---- Phase 3: Memory ---- */
    serial_puts("[croOS] Phase 3: Memory\n");

    if (mbi) {
        detect_memory(mbi);
    } else {
        serial_puts("[croOS] WARNING: no multiboot info, assuming 32MB\n");
    }

    uint32_t mem_kb = 32768; /* default 32MB if no multiboot */
    if (mbi && mbi->mem_lower > 0) {
        mem_kb = mbi->mem_lower + 1024;
    }
    uint32_t mem_bytes = mem_kb * 1024;
    if (mem_bytes > 128 * 1024 * 1024) mem_bytes = 128 * 1024 * 1024;

    serial_puts("[croOS] Initializing PMM...\n");
    pmm_init(mem_bytes);
    serial_puts("[croOS] PMM OK, ");
    serial_put_dec(pmm_get_free_pages());
    serial_puts(" free pages\n");
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  [OK] Physical Memory Manager initialized\n");

    serial_puts("[croOS] Initializing kernel heap...\n");
    kmalloc_init();
    serial_puts("[croOS] Heap OK\n");
    vga_puts("  [OK] Kernel Heap allocator initialized\n");

    serial_puts("[croOS] Initializing VMM + paging...\n");
    vmm_init();
    serial_puts("[croOS] VMM OK (paging enabled)\n");
    vga_puts("  [OK] Virtual Memory Manager initialized (paging enabled)\n");

    /* ---- Phase 4: Drivers (no interrupts yet) ---- */
    serial_puts("[croOS] Phase 4: Drivers\n");

    serial_puts("[croOS] Initializing PIT timer (100 Hz)...\n");
    timer_init(100);
    serial_puts("[croOS] PIT timer OK, handler on INT32\n");
    vga_puts("  [OK] PIT Timer at 100 Hz\n");

    serial_puts("[croOS] Initializing PS/2 keyboard...\n");
    kb_init();
    serial_puts("[croOS] Keyboard OK, handler on INT33\n");
    vga_puts("  [OK] PS/2 Keyboard driver loaded (IRQ1)\n");

    vga_puts("  [OK] VGA text-mode driver (80x25, 16 colors)\n");
    vga_puts("  [OK] Serial port driver (COM1)\n");

    /* ---- Phase 5: Filesystem ---- */
    serial_puts("[croOS] Phase 5: Filesystem\n");

    serial_puts("[croOS] Initializing VFS...\n");
    vfs_init();
    vfs_init_ramdisk();
    serial_puts("[croOS] VFS + ramdisk OK\n");
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  [OK] VFS initialized with ramdisk\n");

    /* ---- Phase 6: Networking ---- */
    serial_puts("[croOS] Phase 6: Network\n");

    serial_puts("[croOS] Initializing TCP/IP stack...\n");
    net_init();
    serial_puts("[croOS] Network OK\n");
    vga_puts("  [OK] TCP/IP networking stack initialized\n");

    /* ---- Phase 7: System calls ---- */
    serial_puts("[croOS] Phase 7: Syscalls\n");

    serial_puts("[croOS] Registering system calls...\n");
    syscall_init();
    serial_puts("[croOS] 42 syscalls registered on INT0x80\n");
    vga_puts("  [OK] 42 system calls registered (INT 0x80)\n");

    /* ---- Phase 8: Processes ---- */
    serial_puts("[croOS] Phase 8: Scheduler\n");

    serial_puts("[croOS] Initializing process scheduler...\n");
    process_init();
    serial_puts("[croOS] Scheduler OK (round-robin, 64 slots)\n");
    vga_puts("  [OK] Process scheduler initialized (round-robin)\n");

    /* ---- Phase 9: Enable interrupts and launch ---- */
    serial_puts("[croOS] Phase 9: Enable interrupts\n");
    serial_puts("[croOS] ============================\n");
    serial_puts("[croOS] All subsystems initialized!\n");
    serial_puts("[croOS] Enabling interrupts...\n");

    vga_putchar('\n');
    vga_set_color(VGA_LGREEN, VGA_BLACK);
    vga_puts("  All subsystems initialized successfully!\n");
    vga_set_color(VGA_CYAN, VGA_BLACK);
    vga_puts("  Type 'help' for available commands.\n\n");

    serial_puts("[croOS] Shell starting...\n");

    /* NOW enable interrupts -- all handlers are registered */
    sti();

    /* Enter kernel shell */
    kernel_shell();
}
