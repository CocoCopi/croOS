/* croOS ramdisk.h — In-memory filesystem (ramdisk) */
#ifndef _RAMDISK_H
#define _RAMDISK_H

#include "fs/vfs.h"

#define RAMDISK_MAX_FILES  256
#define RAMDISK_MAX_SIZE   (64 * 1024 * 1024)  /* 64MB */

void vfs_init_ramdisk(void);
vfs_fs_t *ramdisk_get_fs(void);

#endif
