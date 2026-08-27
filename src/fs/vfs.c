/* croOS vfs.c — Virtual File System
 * Routes file operations to the appropriate filesystem driver.
 * Supports mounting multiple filesystems at different paths. */

#include "kernel/types.h"
#include "fs/vfs.h"
#include "drivers/vga.h"
#include "string.h"

#define MAX_MOUNTS 8

static vfs_fs_t *mounts[MAX_MOUNTS];
static char mount_paths[MAX_MOUNTS][VFS_MAX_PATH];
static int  mount_count = 0;

static vfs_file_t fd_table[VFS_MAX_OPEN];

static int resolve_mount(const char *path, vfs_fs_t **out_fs, char **out_subpath) {
    int best = -1;
    int best_len = 0;
    for (int i = 0; i < mount_count; i++) {
        int len = strlen(mount_paths[i]);
        if (strncmp(path, mount_paths[i], len) == 0) {
            if (len > best_len) {
                best_len = len;
                best = i;
            }
        }
    }
    if (best < 0) return -1;
    *out_fs = mounts[best];
    *out_subpath = (char*)path + best_len;
    if (**out_subpath == '/' && *(*out_subpath + 1) != '\0') (*out_subpath)++;
    return 0;
}

int vfs_get_free_fd(void) {
    for (int i = 0; i < VFS_MAX_OPEN; i++) {
        if (!fd_table[i].in_use) return i;
    }
    return -1;
}

void vfs_init(void) {
    mount_count = 0;
    memset(fd_table, 0, sizeof(fd_table));
    memset(mounts, 0, sizeof(mounts));
}

void vfs_mount(const char *path, vfs_fs_t *fs) {
    if (mount_count >= MAX_MOUNTS) return;
    strncpy(mount_paths[mount_count], path, VFS_MAX_PATH - 1);
    mounts[mount_count] = fs;
    mount_count++;
}

int vfs_open(const char *path, uint8_t mode) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(path, &fs, &sub) < 0) return -1;
    if (!fs->ops.open) return -1;

    int fd = vfs_get_free_fd();
    if (fd < 0) return -1;

    int file_fd = fs->ops.open(sub, mode);
    if (file_fd < 0) return -1;

    fd_table[fd].in_use = 1;
    fd_table[fd].pos = 0;
    fd_table[fd].mode = mode;
    fd_table[fd].data = (void*)(uintptr_t)file_fd;
    strncpy(fd_table[fd].name, path, VFS_MAX_NAME - 1);
    return fd;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !fd_table[fd].in_use) return -1;
    fd_table[fd].in_use = 0;
    return 0;
}

int vfs_read(int fd, void *buf, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !fd_table[fd].in_use) return -1;
    /* Route to root ramdisk for now */
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(fd_table[fd].name, &fs, &sub) < 0) return -1;
    if (!fs->ops.read) return -1;
    int r = fs->ops.read((int)(uintptr_t)fd_table[fd].data, buf, size);
    if (r > 0) fd_table[fd].pos += r;
    return r;
}

int vfs_write(int fd, const void *buf, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !fd_table[fd].in_use) return -1;
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(fd_table[fd].name, &fs, &sub) < 0) return -1;
    if (!fs->ops.write) return -1;
    int w = fs->ops.write((int)(uintptr_t)fd_table[fd].data, buf, size);
    if (w > 0) fd_table[fd].pos += w;
    return w;
}

int vfs_seek(int fd, int32_t offset, int whence) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !fd_table[fd].in_use) return -1;
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(fd_table[fd].name, &fs, &sub) < 0) return -1;
    if (!fs->ops.seek) return -1;
    return fs->ops.seek((int)(uintptr_t)fd_table[fd].data, offset, whence);
}

int vfs_stat(const char *path, vfs_file_t *st) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(path, &fs, &sub) < 0) return -1;
    if (!fs->ops.stat) return -1;
    return fs->ops.stat(sub, st);
}

int vfs_mkdir(const char *path) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(path, &fs, &sub) < 0) return -1;
    if (!fs->ops.mkdir) return -1;
    return fs->ops.mkdir(sub);
}

int vfs_rmdir(const char *path) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(path, &fs, &sub) < 0) return -1;
    if (!fs->ops.rmdir) return -1;
    return fs->ops.rmdir(sub);
}

int vfs_unlink(const char *path) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(path, &fs, &sub) < 0) return -1;
    if (!fs->ops.unlink) return -1;
    return fs->ops.unlink(sub);
}

int vfs_readdir(const char *path, int index, char *out_name) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(path, &fs, &sub) < 0) return -1;
    if (!fs->ops.readdir) return -1;
    return fs->ops.readdir(sub, index, out_name);
}

int vfs_rename(const char *old_path, const char *new_path) {
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(old_path, &fs, &sub) < 0) return -1;
    if (!fs->ops.rename) return -1;
    return fs->ops.rename(sub, new_path);
}

int vfs_truncate(int fd, uint32_t size) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !fd_table[fd].in_use) return -1;
    vfs_fs_t *fs; char *sub;
    if (resolve_mount(fd_table[fd].name, &fs, &sub) < 0) return -1;
    if (!fs->ops.truncate) return -1;
    return fs->ops.truncate((int)(uintptr_t)fd_table[fd].data, size);
}
