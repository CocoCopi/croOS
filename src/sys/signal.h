/* croOS signal.h - POSIX-like signal handling */
#ifndef _SIGNAL_H
#define _SIGNAL_H

#include "kernel/types.h"

#define SIGHUP    1
#define SIGINT    2
#define SIGQUIT   3
#define SIGILL    4
#define SIGTRAP   5
#define SIGABRT   6
#define SIGBUS    7
#define SIGFPE    8
#define SIGKILL   9
#define SIGUSR1   10
#define SIGSEGV   11
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGTERM   15
#define SIGSTKFLT 16
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPWR    30

#define SIG_DFL   0
#define SIG_IGN   1

#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2

typedef void (*signal_handler_t)(int signum);

typedef struct {
    uint32_t mask;          /* signal mask (bit per signal) */
    uint32_t pending;       /* pending signals */
    signal_handler_t handlers[32];
} signal_state_t;

void signal_init(int pid);
int  signal_send(int pid, int signum);
int  signal_handler_set(int signum, signal_handler_t handler);
signal_handler_t signal_handler_get(int signum);
int  signal_mask_set(int how, int signum);
int  signal_check_pending(void);
void signal_deliver(int pid);

#endif
