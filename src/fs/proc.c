/* croOS proc.c - /proc virtual filesystem
 * Exposes kernel state as virtual files: /proc/meminfo, /proc/version,
 * /proc/uptime, /proc/cpuinfo, /proc/[pid]/status, /proc/[pid]/maps. */

#include "kernel/types.h"
#include "proc.h"
#include "kernel/process.h"
#include "drivers/timer.h"
#include "mm/pmm.h"
#include "string.h"

static int proc_open(const char *path, uint8_t mode) {
    (void)mode;
    /* Verify path starts with valid /proc entry */
    if (strcmp(path, "meminfo") == 0) return 0;
    if (strcmp(path, "version") == 0) return 1;
    if (strcmp(path, "uptime") == 0) return 2;
    if (strcmp(path, "cpuinfo") == 0) return 3;
    if (strcmp(path, "loadavg") == 0) return 4;
    if (strcmp(path, "stat") == 0) return 5;
    if (strcmp(path, "modules") == 0) return 6;
    if (strcmp(path, "cmdline") == 0) return 7;
    if (strcmp(path, "hostname") == 0) return 8;
    if (strcmp(path, "filesystems") == 0) return 9;
    if (strcmp(path, "net") == 0) return 10;
    if (strcmp(path, "ioports") == 0) return 11;
    if (strcmp(path, "dma") == 0) return 12;
    if (strcmp(path, "interrupts") == 0) return 13;
    if (strcmp(path, "devices") == 0) return 14;
    if (strcmp(path, "partitions") == 0) return 15;
    if (strcmp(path, "mounts") == 0) return 16;
    return -1;
}

static int proc_read(int fd, void *buf, uint32_t size) {
    char tmp[4096];
    int len = 0;

    switch (fd) {
        case 0: /* /proc/meminfo */
            len = snprintf(tmp, sizeof(tmp),
                "MemTotal:       %u kB\n"
                "MemFree:        %u kB\n"
                "MemAvailable:   %u kB\n"
                "Buffers:        0 kB\n"
                "Cached:         0 kB\n"
                "SwapTotal:      0 kB\n"
                "SwapFree:       0 kB\n"
                "Slab:           0 kB\n"
                "SReclaimable:   0 kB\n"
                "SUnreclaim:     0 kB\n"
                "PageTables:     0 kB\n"
                "VmallocTotal:   0 kB\n",
                pmm_get_total_pages() * 4,
                pmm_get_free_pages() * 4,
                pmm_get_free_pages() * 4);
            break;

        case 1: /* /proc/version */
            len = snprintf(tmp, sizeof(tmp),
                "croOS version 3.0.0 (build@%s %s)\n"
                "gcc version 12.2.0 (i686-linux-gnu)\n"
                "#1 SMP PREEMPT\n",
                __DATE__, __TIME__);
            break;

        case 2: /* /proc/uptime */
            len = snprintf(tmp, sizeof(tmp), "%u.%02u 0.00\n",
                timer_get_seconds(), (timer_get_ticks() % 100));
            break;

        case 3: /* /proc/cpuinfo */
            len = snprintf(tmp, sizeof(tmp),
                "processor\t: 0\n"
                "vendor_id\t: croOS CPU\n"
                "model name\t: croOS Virtual Processor\n"
                "cpu MHz\t\t: 2400.000\n"
                "cache size\t: 256 KB\n"
                "bogomips\t: 4800.00\n"
                "flags\t\t: fpu sse sse2 tsc\n"
                "clflush size\t: 64\n"
                "address sizes\t: 32 bits physical, 32 bits virtual\n");
            break;

        case 4: /* /proc/loadavg */
            len = snprintf(tmp, sizeof(tmp), "0.00 0.00 0.00 1/1 1\n");
            break;

        case 5: /* /proc/stat */
            len = snprintf(tmp, sizeof(tmp),
                "cpu  0 0 0 0 0 0 0 0 0 0\n"
                "cpu0 0 0 0 0 0 0 0 0 0 0\n"
                "intr 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
                "ctxt 0\n"
                "btime %u\n"
                "processes 1\n"
                "procs_running 1\n"
                "procs_blocked 0\n",
                timer_get_seconds());
            break;

        case 6: /* /proc/modules */
            len = snprintf(tmp, sizeof(tmp), "Empty\n");
            break;

        case 7: /* /proc/cmdline */
            len = snprintf(tmp, sizeof(tmp), "croOS root=/dev/sda1\n");
            break;

        case 8: /* /proc/hostname */
            len = snprintf(tmp, sizeof(tmp), "croOS\n");
            break;

        case 9: /* /proc/filesystems */
            len = snprintf(tmp, sizeof(tmp),
                "nodev\tramdisk\n"
                "nodev\tproc\n"
                "nodev\tdev\n"
                "\t fat16\n"
                "\t ext2\n");
            break;

        case 10: /* /proc/net */
            len = snprintf(tmp, sizeof(tmp),
                "Inter-|   Receive                                                Transmit\n"
                " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo frame compressed\n"
                "    lo:       0        0    0    0    0     0          0         0        0        0    0    0    0     0          0\n"
                "  eth0:       0        0    0    0    0     0          0         0        0        0    0    0    0     0          0\n");
            break;

        case 11: /* /proc/ioports */
            len = snprintf(tmp, sizeof(tmp),
                "0000-001f : dma1\n"
                "0020-0021 : pic1\n"
                "0040-0043 : timer0\n"
                "0060-006f : keyboard\n"
                "0070-0071 : rtc\n"
                "0080-008f : dma page reg\n"
                "00a0-00a1 : pic2\n"
                "00c0-00df : dma2\n"
                "0170-0177 : ide1\n"
                "01f0-01f7 : ide0\n"
                "0376-0376 : ide1\n"
                "03b0-03bf : vga+ \n"
                "03c0-03df : vga+ \n"
                "03f0-03f7 : floppy\n"
                "03f6-03f6 : ide0\n"
                "cf8-cff : PCI conf1\n");
            break;

        case 12: /* /proc/dma */
            len = snprintf(tmp, sizeof(tmp), " 4: cascade\n");
            break;

        case 13: /* /proc/interrupts */
            len = snprintf(tmp, sizeof(tmp),
                "           CPU0\n"
                "  0:         0    IO-APIC-edge      timer\n"
                "  1:         0    IO-APIC-edge      keyboard\n"
                "  2:         0    XT-PIC cascade\n"
                "  8:         0    IO-APIC-edge      rtc0\n"
                " 12:         0    IO-APIC-edge      i8042\n"
                " 14:         0    IO-APIC-edge      ide0\n"
                " 15:         0    IO-APIC-edge      ide1\n"
                " 33:         0    IO-APIC-level     keyboard\n"
                " 44:         0    IO-APIC-level     i8042\n"
                "NMI:         0   Non-maskable interrupts\n"
                "LOC:         0   Local timer interrupts\n");
            break;

        case 14: /* /proc/devices */
            len = snprintf(tmp, sizeof(tmp),
                "Character devices:\n"
                "  1 mem\n"
                "  4 tty\n"
                "  5 /dev/tty\n"
                " 10 misc\n"
                " 13 input\n"
                " 29 fb\n"
                "Block devices:\n"
                "  8 sd\n"
                " 11 ide\n"
                " 31 bootdev\n");
            break;

        case 15: /* /proc/partitions */
            len = snprintf(tmp, sizeof(tmp),
                "major minor  #blocks  name\n"
                "   8        0   2048000 sda\n"
                "   8        1   2047969 sda1\n");
            break;

        case 16: /* /proc/mounts */
            len = snprintf(tmp, sizeof(tmp),
                "/dev/sda1 / ext2 rw,relatime 0 0\n"
                "proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0\n"
                "devtmpfs /dev devtmpfs rw,nosuid 0 0\n"
                "tmpfs /tmp tmpfs rw,nosuid,nodev 0 0\n");
            break;

        default:
            len = snprintf(tmp, sizeof(tmp), "Unknown /proc entry\n");
            break;
    }

    uint32_t to_copy = (uint32_t)len < size ? (uint32_t)len : size;
    memcpy(buf, tmp, to_copy);
    return (int)to_copy;
}

static int proc_close(int fd) {
    (void)fd;
    return 0;
}

static int proc_readdir(const char *path, int index, char *out_name) {
    (void)path;
    static const char *entries[] = {
        "meminfo", "version", "uptime", "cpuinfo", "loadavg",
        "stat", "modules", "cmdline", "hostname", "filesystems",
        "net", "ioports", "dma", "interrupts", "devices",
        "partitions", "mounts", NULL
    };
    if (index < 0 || !entries[index]) return -1;
    strcpy(out_name, entries[index]);
    return 0;
}

void proc_init(void) {
    /* /proc is initialized but not mounted by default */
}

void proc_register(vfs_fs_t *vfs) {
    strcpy(vfs->name, "proc");
    vfs->ops.open    = proc_open;
    vfs->ops.close   = proc_close;
    vfs->ops.read    = proc_read;
    vfs->ops.readdir = proc_readdir;
}
