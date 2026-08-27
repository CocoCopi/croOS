/* croOS vfs.h — Virtual File System layer */
#ifndef _VFS_H
#define _VFS_H

#include "kernel/types.h"

#define VFS_MAX_NAME    64
#define VFS_MAX_PATH    256
#define VFS_MAX_FILES   1024
#define VFS_MAX_OPEN    64

#define VFS_TYPE_FILE    1
#define VFS_TYPE_DIR     2
#define VFS_TYPE_CHARDEV 3
#define VFS_TYPE_BLOCKDEV 4
#define VFS_TYPE_PIPE    5
#define VFS_TYPE_SYMLINK 6
#define VFS_TYPE_SOCKET  7

#define VFS_MODE_READ    0x01
#define VFS_MODE_WRITE   0x02
#define VFS_MODE_APPEND  0x04
#define VFS_MODE_CREATE  0x08

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* File descriptor */
typedef struct {
    char    name[VFS_MAX_NAME];
    uint8_t type;
    uint32_t size;
    uint32_t pos;
    uint8_t  mode;
    uint8_t  in_use;
    void    *data;      /* filesystem-specific data */
    uint32_t inode;
} vfs_file_t;

/* Filesystem operations */
typedef struct {
    int   (*open)(const char *path, uint8_t mode);
    int   (*close)(int fd);
    int   (*read)(int fd, void *buf, uint32_t size);
    int   (*write)(int fd, const void *buf, uint32_t size);
    int   (*seek)(int fd, int32_t offset, int whence);
    int   (*stat)(const char *path, vfs_file_t *st);
    int   (*mkdir)(const char *path);
    int   (*rmdir)(const char *path);
    int   (*unlink)(const char *path);
    int   (*readdir)(const char *path, int index, char *out_name);
    int   (*rename)(const char *old_path, const char *new_path);
    int   (*truncate)(int fd, uint32_t size);
} vfs_ops_t;

/* Filesystem registration */
typedef struct {
    char     name[16];
    vfs_ops_t ops;
    uint32_t  mount_point;  /* base address or device */
} vfs_fs_t;

void vfs_init(void);
void vfs_mount(const char *path, vfs_fs_t *fs);
int  vfs_open(const char *path, uint8_t mode);
int  vfs_close(int fd);
int  vfs_read(int fd, void *buf, uint32_t size);
int  vfs_write(int fd, const void *buf, uint32_t size);
int  vfs_seek(int fd, int32_t offset, int whence);
int  vfs_stat(const char *path, vfs_file_t *st);
int  vfs_mkdir(const char *path);
int  vfs_rmdir(const char *path);
int  vfs_unlink(const char *path);
int  vfs_readdir(const char *path, int index, char *out_name);
int  vfs_rename(const char *old_path, const char *new_path);
int  vfs_truncate(int fd, uint32_t size);
int  vfs_get_free_fd(void);

#endif
