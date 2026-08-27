/* croOS signal.c - Signal delivery and handler management
 * Manages signal masks, pending signals, and handler dispatch per-process. */

#include "kernel/types.h"
#include "signal.h"
#include "kernel/process.h"
#include "drivers/vga.h"
#include "string.h"

static signal_state_t signals[MAX_PROCS];

void signal_init(int pid) {
    if (pid < 0 || pid >= MAX_PROCS) return;
    memset(&signals[pid], 0, sizeof(signal_state_t));
    for (int i = 0; i < 32; i++)
        signals[pid].handlers[i] = (signal_handler_t)SIG_DFL;
}

int signal_send(int pid, int signum) {
    if (pid < 0 || pid >= MAX_PROCS || signum < 1 || signum > 31) return -1;

    /* Set pending bit */
    signals[pid].pending |= (1 << signum);

    /* Special signals */
    if (signum == SIGKILL) {
        process_exit(pid, signum);
        return 0;
    }
    if (signum == SIGSTOP) {
        process_t *p = process_get(pid);
        if (p) p->state = PROC_SLEEPING;
        return 0;
    }
    if (signum == SIGCONT) {
        process_t *p = process_get(pid);
        if (p) p->state = PROC_RUNNING;
        return 0;
    }
    return 0;
}

int signal_handler_set(int signum, signal_handler_t handler) {
    if (signum < 1 || signum > 31) return -1;
    int pid = process_get_pid();
    if (pid < 0 || pid >= MAX_PROCS) return -1;
    signals[pid].handlers[signum] = handler;
    return 0;
}

signal_handler_t signal_handler_get(int signum) {
    if (signum < 1 || signum > 31) return (signal_handler_t)SIG_DFL;
    int pid = process_get_pid();
    if (pid < 0 || pid >= MAX_PROCS) return (signal_handler_t)SIG_DFL;
    return signals[pid].handlers[signum];
}

int signal_mask_set(int how, int signum) {
    if (signum < 1 || signum > 31) return -1;
    int pid = process_get_pid();
    if (pid < 0 || pid >= MAX_PROCS) return -1;

    switch (how) {
        case SIG_BLOCK:
            signals[pid].mask |= (1 << signum);
            break;
        case SIG_UNBLOCK:
            signals[pid].mask &= ~(1 << signum);
            break;
        case SIG_SETMASK:
            signals[pid].mask = (1 << signum);
            break;
        default:
            return -1;
    }
    return 0;
}

int signal_check_pending(void) {
    int pid = process_get_pid();
    if (pid < 0 || pid >= MAX_PROCS) return -1;

    /* Find first unmasked pending signal */
    uint32_t pending = signals[pid].pending & ~signals[pid].mask;
    for (int i = 1; i < 32; i++) {
        if (pending & (1 << i)) return i;
    }
    return 0;
}

void signal_deliver(int pid) {
    if (pid < 0 || pid >= MAX_PROCS) return;

    uint32_t pending = signals[pid].pending & ~signals[pid].mask;
    for (int i = 1; i < 32; i++) {
        if (pending & (1 << i)) {
            signals[pid].pending &= ~(1 << i);

            signal_handler_t handler = signals[pid].handlers[i];
            if (handler == (signal_handler_t)SIG_IGN) continue;
            if (handler == (signal_handler_t)SIG_DFL) {
                /* Default action varies by signal */
                switch (i) {
                    case SIGINT: case SIGQUIT: case SIGTERM:
                    case SIGKILL: case SIGSEGV:
                        process_exit(pid, i);
                        break;
                    default:
                        break;
                }
            } else {
                /* Call user handler */
                handler(i);
            }
        }
    }
}
