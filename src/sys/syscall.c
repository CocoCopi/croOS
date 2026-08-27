/* croOS syscall.c — System call handler
 * Dispatches INT 0x80 to appropriate kernel function.
 * Uses register passing convention: EAX=syscall#, EBX/ECX/EDX=args. */

#include "kernel/types.h"
#include "sys/syscall.h"
#include "fs/vfs.h"
#include "drivers/vga.h"
#include "drivers/keyboard.h"
#include "drivers/timer.h"
#include "mm/kmalloc.h"
#include "kernel/idt.h"
#include "string.h"

/* Forward declaration */
uint32_t syscall_dispatch(regs_t *r);

static int current_pid = 1;

/* All syscall handlers use the same 3-arg signature */
static uint32_t sys_exit(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    vga_puts("\n[Process exited]\n");
    return 0;
}

static uint32_t sys_read(uint32_t fd, uint32_t buf, uint32_t size) {
    return (uint32_t)vfs_read((int)fd, (void*)buf, size);
}

static uint32_t sys_write(uint32_t fd, uint32_t buf, uint32_t size) {
    return (uint32_t)vfs_write((int)fd, (void*)buf, size);
}

static uint32_t sys_open(uint32_t path, uint32_t mode, uint32_t c) {
    (void)c;
    return (uint32_t)vfs_open((const char*)path, (uint8_t)mode);
}

static uint32_t sys_close(uint32_t fd, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    return (uint32_t)vfs_close((int)fd);
}

static uint32_t sys_seek(uint32_t fd, uint32_t offset, uint32_t whence) {
    return (uint32_t)vfs_seek((int)fd, (int32_t)offset, (int)whence);
}

static uint32_t sys_mkdir(uint32_t path, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    return (uint32_t)vfs_mkdir((const char*)path);
}

static uint32_t sys_rmdir(uint32_t path, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    return (uint32_t)vfs_rmdir((const char*)path);
}

static uint32_t sys_unlink(uint32_t path, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    return (uint32_t)vfs_unlink((const char*)path);
}

static uint32_t sys_readdir(uint32_t path, uint32_t index, uint32_t out_name) {
    return (uint32_t)vfs_readdir((const char*)path, (int)index, (char*)out_name);
}

static uint32_t sys_getpid(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    return (uint32_t)current_pid;
}

static uint32_t sys_sleep(uint32_t ms, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    timer_sleep(ms);
    return 0;
}

static uint32_t sys_tick(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    return (uint32_t)timer_get_ticks();
}

static uint32_t sys_vgawrite(uint32_t str, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    vga_puts((const char*)str);
    return 0;
}

static uint32_t sys_vgaclear(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    vga_clear();
    return 0;
}

static uint32_t sys_vgacolor(uint32_t fg, uint32_t bg, uint32_t c) {
    (void)c;
    vga_set_color((uint8_t)fg, (uint8_t)bg);
    return 0;
}

static uint32_t sys_kbget(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    return (uint32_t)kb_getchar();
}

static uint32_t sys_alloc(uint32_t size, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    return (uint32_t)kmalloc(size);
}

static uint32_t sys_free(uint32_t ptr, uint32_t b, uint32_t c) {
    (void)b; (void)c;
    kfree((void*)ptr);
    return 0;
}

static uint32_t sys_rename(uint32_t oldp, uint32_t newp, uint32_t c) {
    (void)c;
    return (uint32_t)vfs_rename((const char*)oldp, (const char*)newp);
}

static uint32_t sys_truncate(uint32_t fd, uint32_t size, uint32_t c) {
    (void)c;
    return (uint32_t)vfs_truncate((int)fd, size);
}

static uint32_t sys_reboot(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    outb(0x92, 0x03);
    while(1) hlt();
    return 0;
}

static uint32_t sys_shutdown(uint32_t a, uint32_t b, uint32_t c) {
    (void)a; (void)b; (void)c;
    outw(0x604, 0x2000);
    outw(0xB004, 0x2000);
    while(1) hlt();
    return 0;
}

typedef uint32_t (*syscall_fn_t)(uint32_t, uint32_t, uint32_t);

static syscall_fn_t syscall_table[MAX_SYSCALLS] = {
    [SYS_EXIT]     = sys_exit,
    [SYS_READ]     = sys_read,
    [SYS_WRITE]    = sys_write,
    [SYS_OPEN]     = sys_open,
    [SYS_CLOSE]    = sys_close,
    [SYS_SEEK]     = sys_seek,
    [SYS_MKDIR]    = sys_mkdir,
    [SYS_RMDIR]    = sys_rmdir,
    [SYS_UNLINK]   = sys_unlink,
    [SYS_REaddir]  = sys_readdir,
    [SYS_GETPID]   = sys_getpid,
    [SYS_SLEEP]    = sys_sleep,
    [SYS_TICK]     = sys_tick,
    [SYS_VGAWRITE] = sys_vgawrite,
    [SYS_VGACLEAR] = sys_vgaclear,
    [SYS_VGACOLOR] = sys_vgacolor,
    [SYS_KBGET]    = sys_kbget,
    [SYS_ALLOC]    = sys_alloc,
    [SYS_FREE]     = sys_free,
    [SYS_RENAME]   = sys_rename,
    [SYS_TRUNCATE] = sys_truncate,
    [SYS_REBOOT]   = sys_reboot,
    [SYS_SHUTDOWN] = sys_shutdown,
};

static void syscall_handler(regs_t *r) {
    r->eax = syscall_dispatch(r);
}

uint32_t syscall_dispatch(regs_t *r) {
    uint32_t num = r->eax;
    if (num >= MAX_SYSCALLS || !syscall_table[num]) {
        vga_puts("[SYSCALL] Unknown syscall #");
        vga_put_dec(num);
        return (uint32_t)-1;
    }
    return syscall_table[num](r->ebx, r->ecx, r->edx);
}

void syscall_init(void) {
    isr_install_handler(0x80, syscall_handler);
}
