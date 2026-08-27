/* croOS syscall.h — System call numbers and interface */
#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "kernel/types.h"
#include "kernel/idt.h"

/* System call numbers */
#define SYS_EXIT      0
#define SYS_READ      1
#define SYS_WRITE     2
#define SYS_OPEN      3
#define SYS_CLOSE     4
#define SYS_SEEK      5
#define SYS_STAT      6
#define SYS_MKDIR     7
#define SYS_RMDIR     8
#define SYS_UNLINK    9
#define SYS_REaddir   10
#define SYS_RENAME    11
#define SYS_TRUNCATE  12
#define SYS_FORK      13
#define SYS_EXEC      14
#define SYS_WAIT      15
#define SYS_GETPID    16
#define SYS_SBRK      17
#define SYS_SLEEP     18
#define SYS_TICK      19
#define SYS_GETTOD    20
#define SYS_VGAWRITE  21
#define SYS_VGACLEAR  22
#define SYS_VGACOLOR  23
#define SYS_KBGET     24
#define SYS_ALLOC     25
#define SYS_FREE      26
#define SYS_SOCKET    27
#define SYS_CONNECT   28
#define SYS_SEND      29
#define SYS_RECV      30
#define SYS_IOCTL     31
#define SYS_SHUTDOWN  32
#define SYS_REBOOT    33
#define SYS_LSPROCS   34
#define SYS_TOP       35
#define SYS_MOUNT     36
#define SYS_UNMOUNT   37
#define SYS_CHMOD     38
#define SYS_CHOWN     39
#define SYS_SYMLINK   40
#define SYS_READLINK  41

#define MAX_SYSCALLS 42

void syscall_init(void);
uint32_t syscall_dispatch(regs_t *r);

#endif
