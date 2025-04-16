#pragma once
#ifndef __KERNEL__
#include "kernel/include/kernel_signal.h"
#include <sys/types.h>
int
kill(pid_t pid, int signal);
sighandler_t
signal(int signum, sighandler_t handler);
// Seems strange, but everyone defines it this way.
// Int modifications, at least on x86, are an atomic transaction.
typedef int sig_atomic_t;
int
sigemptyset(sigset_t *set);
int
sigfillset(sigset_t *set);
int
sigaddset(sigset_t *set, int signum);
int
sigdelset(sigset_t *set, int signum);
int
sigismember(const sigset_t *set, int signum);

int
sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset);
int
raise(int sig);
int
sigsuspend(const sigset_t *mask);
int
sigaction(int signum, const struct sigaction *restrict act,
		struct sigaction *restrict oldact);
void
psignal(int sig, const char *s);
#endif

