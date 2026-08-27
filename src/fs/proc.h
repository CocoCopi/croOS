/* croOS proc.h - /proc virtual filesystem */
#ifndef _PROC_H
#define _PROC_H

#include "vfs.h"

void proc_init(void);
void proc_register(vfs_fs_t *vfs);

#endif
