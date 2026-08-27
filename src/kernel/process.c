/* croOS process.c — Process management and round-robin scheduler
 * Creates/destroys processes, context-switches on timer ticks,
 * supports forking, exec, and basic process accounting. */

#include "kernel/types.h"
#include "kernel/process.h"
#include "kernel/idt.h"
#include "mm/kmalloc.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "drivers/vga.h"
#include "drivers/timer.h"
#include "string.h"

static process_t procs[MAX_PROCS];
static int next_pid = 1;
static int current_proc_idx = 0;
static int proc_count = 0;

/* Assembly: context switch */
extern void ctx_switch(proc_regs_t *old, proc_regs_t *new_regs);

void process_init(void) {
    memset(procs, 0, sizeof(procs));
    next_pid = 1;
    proc_count = 0;

    /* Create idle process (PID 1) */
    process_create("idle", 0, 1);
}

int process_create(const char *name, uint32_t entry, uint32_t priority) {
    if (proc_count >= MAX_PROCS) return -1;

    int idx = -1;
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].state == PROC_UNUSED) { idx = i; break; }
    }
    if (idx < 0) return -1;

    process_t *p = &procs[idx];
    memset(p, 0, sizeof(process_t));

    p->pid = next_pid++;
    p->ppid = process_get_pid();
    p->state = PROC_RUNNING;
    p->priority = priority ? priority : 10;
    p->ticks_left = p->priority;
    p->start_time = timer_get_ticks();

    strncpy(p->name, name, PROC_NAME_LEN - 1);

    /* Create page directory */
    p->page_dir = vmm_create_dir();

    /* Allocate kernel stack */
    void *kstack = pmm_alloc_page();
    p->kernel_stack = (uint32_t)kstack + PAGE_SIZE;

    /* Allocate user stack (at 0xBFFFE000) */
    p->user_stack = 0xBFFFE000;
    void *ustack_page = pmm_alloc_page();
    if (ustack_page) {
        vmm_map_page(p->page_dir, p->user_stack - PAGE_SIZE,
                     (uint32_t)ustack_page,
                     PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    p->brk = 0x01000000;  /* 16MB initial heap */

    /* Set up initial registers */
    memset(&p->saved_regs, 0, sizeof(proc_regs_t));
    p->saved_regs.eip = entry;
    p->saved_regs.esp = p->user_stack;
    p->saved_regs.ebp = p->user_stack;
    p->saved_regs.cs  = 0x1B;  /* user code segment */
    p->saved_regs.ds  = 0x23;
    p->saved_regs.ss  = 0x23;
    p->saved_regs.eflags = 0x202;  /* IF=1 */

    /* Initialize file descriptors (inherit stdin/stdout/stderr) */
    for (int i = 0; i < MAX_FD_PER_PROC; i++) p->fd_table[i] = -1;

    proc_count++;
    return p->pid;
}

void process_exit(int pid, uint32_t code) {
    (void)code;
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == pid && procs[i].state != PROC_UNUSED) {
            /* Free page directory */
            if (procs[i].page_dir) {
                /* TODO: free all page tables */
            }
            procs[i].state = PROC_UNUSED;
            procs[i].pid = 0;
            proc_count--;
            return;
        }
    }
}

int process_fork(int parent_pid) {
    int parent_idx = -1;
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == parent_pid) { parent_idx = i; break; }
    }
    if (parent_idx < 0) return -1;

    process_t *parent = &procs[parent_idx];

    /* Create child process */
    int child_pid = process_create("child", 0, parent->priority);
    if (child_pid < 0) return -1;

    int child_idx = -1;
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == child_pid) { child_idx = i; break; }
    }

    process_t *child = &procs[child_idx];
    child->ppid = parent_pid;

    /* Copy registers */
    memcpy(&child->saved_regs, &parent->saved_regs, sizeof(proc_regs_t));
    child->saved_regs.eax = 0;  /* child returns 0 from fork */

    /* Copy page directory (COW) */
    child->page_dir = vmm_create_dir();
    child->brk = parent->brk;

    return child_pid;
}

int process_exec(int pid, uint32_t entry) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == pid) {
            memset(&procs[i].saved_regs, 0, sizeof(proc_regs_t));
            procs[i].saved_regs.eip = entry;
            procs[i].saved_regs.esp = procs[i].user_stack;
            procs[i].saved_regs.ebp = procs[i].user_stack;
            procs[i].saved_regs.cs  = 0x1B;
            procs[i].saved_regs.ds  = 0x23;
            procs[i].saved_regs.eflags = 0x202;
            procs[i].state = PROC_RUNNING;
            return 0;
        }
    }
    return -1;
}

/* Called from timer IRQ — schedules the next process */
void process_scheduler(regs_t *r) {
    if (proc_count == 0) return;

    /* Save current process registers */
    procs[current_proc_idx].saved_regs.eax = r->eax;
    procs[current_proc_idx].saved_regs.ebx = r->ebx;
    procs[current_proc_idx].saved_regs.ecx = r->ecx;
    procs[current_proc_idx].saved_regs.edx = r->edx;
    procs[current_proc_idx].saved_regs.esi = r->esi;
    procs[current_proc_idx].saved_regs.edi = r->edi;
    procs[current_proc_idx].saved_regs.ebp = r->ebp;
    procs[current_proc_idx].saved_regs.esp = r->esp;
    procs[current_proc_idx].saved_regs.eip = r->eip;
    procs[current_proc_idx].saved_regs.eflags = r->eflags;
    procs[current_proc_idx].saved_regs.cs  = r->cs;
    procs[current_proc_idx].saved_regs.ss  = r->ss;
    procs[current_proc_idx].saved_regs.ds  = r->ds;

    procs[current_proc_idx].total_ticks++;

    /* Find next runnable process (round-robin with priority) */
    int next = current_proc_idx;
    for (int i = 0; i < MAX_PROCS; i++) {
        next = (next + 1) % MAX_PROCS;
        if (procs[next].state == PROC_RUNNING || procs[next].state == PROC_READY)
            break;
    }

    /* Switch page directory */
    if (procs[next].page_dir != procs[current_proc_idx].page_dir) {
        vmm_switch_dir(procs[next].page_dir);
    }

    current_proc_idx = next;

    /* Restore registers */
    r->eax = procs[next].saved_regs.eax;
    r->ebx = procs[next].saved_regs.ebx;
    r->ecx = procs[next].saved_regs.ecx;
    r->edx = procs[next].saved_regs.edx;
    r->esi = procs[next].saved_regs.esi;
    r->edi = procs[next].saved_regs.edi;
    r->ebp = procs[next].saved_regs.ebp;
    r->esp = procs[next].saved_regs.esp;
    r->eip = procs[next].saved_regs.eip;
    r->eflags = procs[next].saved_regs.eflags;
    r->cs  = procs[next].saved_regs.cs;
    r->ss  = procs[next].saved_regs.ss;
    r->ds  = procs[next].saved_regs.ds;
}

int process_get_pid(void) {
    if (current_proc_idx >= 0 && current_proc_idx < MAX_PROCS)
        return procs[current_proc_idx].pid;
    return 1;
}

process_t *process_get(int pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].pid == pid && procs[i].state != PROC_UNUSED)
            return &procs[i];
    }
    return NULL;
}

int process_list(char *buf, int bufsize) {
    int pos = 0;
    pos += snprintf(buf + pos, bufsize - pos, "PID  NAME               STATE     CPU\n");
    pos += snprintf(buf + pos, bufsize - pos, "---- ------------------ --------- ----\n");
    for (int i = 0; i < MAX_PROCS; i++) {
        if (procs[i].state != PROC_UNUSED) {
            const char *state = "running";
            if (procs[i].state == PROC_SLEEPING) state = "sleeping";
            if (procs[i].state == PROC_ZOMBIE)   state = "zombie";
            pos += snprintf(buf + pos, bufsize - pos, "%-4d %-18s %-9s %d\n",
                procs[i].pid, procs[i].name, state, procs[i].total_ticks);
        }
    }
    return pos;
}
