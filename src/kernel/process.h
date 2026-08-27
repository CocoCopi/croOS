/* croOS process.h — Process and scheduler */
#ifndef _PROCESS_H
#define _PROCESS_H

#include "kernel/types.h"
#include "kernel/idt.h"
#include "mm/vmm.h"

#define MAX_PROCS    64
#define PROC_NAME_LEN 32
#define PROC_STACK_SIZE (16 * 1024)  /* 16KB kernel stack per process */
#define MAX_FD_PER_PROC 32

#define PROC_UNUSED   0
#define PROC_RUNNING  1
#define PROC_READY    2
#define PROC_SLEEPING 3
#define PROC_ZOMBIE   4

typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ss, ds, es, fs, gs;
} __packed proc_regs_t;

typedef struct {
    int        pid;
    int        ppid;
    char       name[PROC_NAME_LEN];
    uint8_t    state;
    uint32_t   priority;   /* higher = more CPU time */
    uint32_t   ticks_left;

    /* Memory */
    page_dir_t *page_dir;
    uint32_t    kernel_stack;
    uint32_t    user_stack;
    uint32_t    brk;       /* program break (heap end) */

    /* File descriptors */
    int        fd_table[MAX_FD_PER_PROC];
    int        fd_count;

    /* Scheduling */
    proc_regs_t saved_regs;

    /* Signals */
    uint32_t    signal_pending;

    /* Accounting */
    uint32_t    total_ticks;
    uint32_t    start_time;
} process_t;

void process_init(void);
int  process_create(const char *name, uint32_t entry, uint32_t priority);
void process_exit(int pid, uint32_t code);
int  process_fork(int parent_pid);
int  process_exec(int pid, uint32_t entry);
void process_scheduler(regs_t *r);
int  process_get_pid(void);
process_t *process_get(int pid);
int  process_list(char *buf, int bufsize);

#endif
