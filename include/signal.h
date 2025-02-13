#pragma once
#ifndef __KERNEL__
#include "kernel/include/kernel_signal.h"
#include <sys/types.h>
int
kill(pid_t pid, int signal);
sighandler_t
signal(int signum, sighandler_t handler);
typedef int sig_atomic_t;
#endif

