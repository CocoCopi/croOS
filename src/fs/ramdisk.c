/* croOS ramdisk.c — In-memory filesystem
 * Stores files in memory as a flat array of file entries.
 * Supports create, read, write, delete, mkdir, readdir. */

#include "kernel/types.h"
#include "fs/ramdisk.h"
#include "mm/kmalloc.h"
#include "drivers/vga.h"
#include "string.h"

#define RD_MAX_NAME 64

typedef struct {
    char     name[RD_MAX_NAME];
    uint32_t size;
    uint32_t capacity;
    uint8_t  in_use;
    uint8_t  is_dir;
    void    *data;
    int      parent;
} rd_entry_t;

static rd_entry_t rd_files[RAMDISK_MAX_FILES];
static int rd_count = 0;
static int rd_root = -1;
static int rd_open_fds[64];  /* fd → file index mapping */
static int rd_open_pos[64];
static int rd_open_modes[64];

static int rd_find(const char *name) {
    for (int i = 0; i < rd_count; i++) {
        if (rd_files[i].in_use && strcmp(rd_files[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int rd_create(const char *name, uint8_t is_dir) {
    if (rd_count >= RAMDISK_MAX_FILES) return -1;
    int idx = rd_count++;
    memset(&rd_files[idx], 0, sizeof(rd_entry_t));
    strncpy(rd_files[idx].name, name, RD_MAX_NAME - 1);
    rd_files[idx].in_use = 1;
    rd_files[idx].is_dir = is_dir;
    rd_files[idx].parent = rd_root;
    return idx;
}

static int ramdisk_open(const char *path, uint8_t mode) {
    /* Skip leading / */
    while (*path == '/') path++;
    if (*path == '\0') path = ".";

    int idx = rd_find(path);
    if (idx < 0 && (mode & VFS_MODE_CREATE)) {
        idx = rd_create(path, 0);
        if (idx < 0) return -1;
    }
    if (idx < 0) return -1;

    /* Find free fd slot */
    for (int fd = 0; fd < 64; fd++) {
        if (rd_open_fds[fd] == -1) {
            rd_open_fds[fd] = idx;
            rd_open_pos[fd] = 0;
            rd_open_modes[fd] = mode;
            return fd;
        }
    }
    return -1;
}

static int ramdisk_close(int fd) {
    if (fd < 0 || fd >= 64 || rd_open_fds[fd] == -1) return -1;
    rd_open_fds[fd] = -1;
    return 0;
}

static int ramdisk_read(int fd, void *buf, uint32_t size) {
    if (fd < 0 || fd >= 64 || rd_open_fds[fd] == -1) return -1;
    int idx = rd_open_fds[fd];
    int pos = rd_open_pos[fd];
    if (pos >= (int)rd_files[idx].size) return 0;
    uint32_t avail = rd_files[idx].size - pos;
    uint32_t to_read = size < avail ? size : avail;
    memcpy(buf, (uint8_t*)rd_files[idx].data + pos, to_read);
    rd_open_pos[fd] += to_read;
    return (int)to_read;
}

static int ramdisk_write(int fd, const void *buf, uint32_t size) {
    if (fd < 0 || fd >= 64 || rd_open_fds[fd] == -1) return -1;
    int idx = rd_open_fds[fd];
    int pos = rd_open_pos[fd];

    /* Expand buffer if needed */
    if (pos + (int)size > (int)rd_files[idx].capacity) {
        uint32_t new_cap = (pos + size + 4095) & ~4095;
        void *new_data = kmalloc(new_cap);
        if (!new_data) return -1;
        if (rd_files[idx].data) {
            memcpy(new_data, rd_files[idx].data, rd_files[idx].size);
            kfree(rd_files[idx].data);
        }
        rd_files[idx].data = new_data;
        rd_files[idx].capacity = new_cap;
    }

    memcpy((uint8_t*)rd_files[idx].data + pos, buf, size);
    rd_open_pos[fd] += size;
    if ((uint32_t)rd_open_pos[fd] > rd_files[idx].size)
        rd_files[idx].size = rd_open_pos[fd];
    return (int)size;
}

static int ramdisk_seek(int fd, int32_t offset, int whence) {
    if (fd < 0 || fd >= 64 || rd_open_fds[fd] == -1) return -1;
    int idx = rd_open_fds[fd];
    int new_pos = 0;
    switch (whence) {
        case SEEK_SET: new_pos = offset; break;
        case SEEK_CUR: new_pos = rd_open_pos[fd] + offset; break;
        case SEEK_END: new_pos = (int)rd_files[idx].size + offset; break;
        default: return -1;
    }
    if (new_pos < 0) new_pos = 0;
    if ((uint32_t)new_pos > rd_files[idx].size) new_pos = rd_files[idx].size;
    rd_open_pos[fd] = new_pos;
    return new_pos;
}

static int ramdisk_mkdir(const char *path) {
    while (*path == '/') path++;
    return rd_create(path, 1);
}

static int ramdisk_rmdir(const char *path) {
    while (*path == '/') path++;
    int idx = rd_find(path);
    if (idx < 0) return -1;
    if (!rd_files[idx].is_dir) return -1;
    rd_files[idx].in_use = 0;
    return 0;
}

static int ramdisk_unlink(const char *path) {
    while (*path == '/') path++;
    int idx = rd_find(path);
    if (idx < 0) return -1;
    if (rd_files[idx].data) kfree(rd_files[idx].data);
    rd_files[idx].in_use = 0;
    return 0;
}

static int ramdisk_stat(const char *path, vfs_file_t *st) {
    while (*path == '/') path++;
    int idx = rd_find(path);
    if (idx < 0) return -1;
    strncpy(st->name, rd_files[idx].name, VFS_MAX_NAME - 1);
    st->size = rd_files[idx].size;
    st->type = rd_files[idx].is_dir ? VFS_TYPE_DIR : VFS_TYPE_FILE;
    return 0;
}

static int ramdisk_readdir(const char *path, int index, char *out_name) {
    (void)path;
    int count = 0;
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (rd_files[i].in_use) {
            if (count == index) {
                strncpy(out_name, rd_files[i].name, RD_MAX_NAME);
                return 0;
            }
            count++;
        }
    }
    return -1;
}

static int ramdisk_truncate(int fd, uint32_t size) {
    if (fd < 0 || fd >= 64 || rd_open_fds[fd] == -1) return -1;
    int idx = rd_open_fds[fd];
    rd_files[idx].size = size;
    return 0;
}

static vfs_fs_t ramdisk_fs = {
    .name = "ramdisk",
    .ops = {
        .open    = ramdisk_open,
        .close   = ramdisk_close,
        .read    = ramdisk_read,
        .write   = ramdisk_write,
        .seek    = ramdisk_seek,
        .stat    = ramdisk_stat,
        .mkdir   = ramdisk_mkdir,
        .rmdir   = ramdisk_rmdir,
        .unlink  = ramdisk_unlink,
        .readdir = ramdisk_readdir,
        .rename  = 0,
        .truncate = ramdisk_truncate,
    }
};

void vfs_init_ramdisk(void) {
    for (int i = 0; i < 64; i++) rd_open_fds[i] = -1;
    rd_count = 0;
    rd_root = rd_create("/", 1);
    vfs_mount("/", &ramdisk_fs);

    /* Create standard directories */
    ramdisk_mkdir("/bin");
    ramdisk_mkdir("/dev");
    ramdisk_mkdir("/etc");
    ramdisk_mkdir("/home");
    ramdisk_mkdir("/tmp");
    ramdisk_mkdir("/var");
    ramdisk_mkdir("/usr");
    ramdisk_mkdir("/usr/lib");
}

vfs_fs_t *ramdisk_get_fs(void) { return &ramdisk_fs; }
