/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

sighandler_t
signal(int signum, sighandler_t handler)
{
	return (sighandler_t)__syscall_ret(
		__syscall2(SYS_signal, signum, (long)handler));
}

int
kill(pid_t pid, int sig)
{
	return __syscall_ret(__syscall2(SYS_kill, pid, sig));
}

int
killpg(pid_t pgid, int sig)
{
	return kill(-pgid, sig);
}

int
sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset)
{
	return __syscall_ret(
		__syscall3(SYS_sigprocmask, how, (long)set, (long)oldset));
}

int
sigsuspend(const sigset_t *mask)
{
	return __syscall_ret(__syscall1(SYS_sigsuspend, (long)mask));
}

int
sigaction(int signum, const struct sigaction *restrict act,
          struct sigaction *restrict oldact)
{
	return __syscall_ret(
		__syscall3(SYS_sigaction, signum, (long)act, (long)oldact));
}

int
sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}

int
sigfillset(sigset_t *set)
{
	*set = ~0U;
	return 0;
}

int
sigaddset(sigset_t *set, int signum)
{
	if (signum < 0 || signum > NSIG) {
		errno = EINVAL;
		return -1;
	}
	*set |= __SIG_BIT(signum);
	return 0;
}

int
sigdelset(sigset_t *set, int signum)
{
	if (signum < 0 || signum > NSIG) {
		errno = EINVAL;
		return -1;
	}
	*set &= ~__SIG_BIT(signum);
	return 0;
}

int
sigismember(const sigset_t *set, int signum)
{
	if (signum < 0 || signum > NSIG) {
		errno = EINVAL;
		return -1;
	}
	return (*set & __SIG_BIT(signum)) != 0;
}

static const char *signal_map[] = {
	[0] = "Success",
	[SIGHUP] = "Hangup",
	[SIGINT] = "Interrupt",
	[SIGQUIT] = "Quit",
	[SIGILL] = "Illegal instruction",
	[SIGTRAP] = "Trace/breakpoint trap",
	[SIGABRT] = "Aborted",
	[SIGBUS] = "Bus error",
	[SIGFPE] = "Floating point exception",
	[SIGKILL] = "Killed",
	[SIGUSR1] = "User defined signal 1",
	[SIGSEGV] = "Segmentation fault",
	[SIGUSR2] = "User defined signal 2",
	[SIGPIPE] = "Broken pipe",
	[SIGALRM] = "Alarm clock",
	[SIGTERM] = "Terminated",
	[SIGSTKFLT] = "Stack fault",
	[SIGCHLD] = "Child process status",
	[SIGCONT] = "Continued (signal)",
	[SIGSTOP] = "Stopped (signal)",
	[SIGTSTP] = "Stopped",
	[SIGTTIN] = "Stopped (tty input)",
	[SIGTTOU] = "Stopped (tty output)",
	[SIGURG] = "Urgent I/O condition",
	[SIGXCPU] = "CPU time limit exceeded",
	[SIGXFSZ] = "File size limit exceeded",
	[SIGVTALRM] = "Virtual timer expired",
	[SIGPROF] = "Profiling timer expired",
	[SIGWINCH] = "Window changed",
	[SIGIO] = "I/O possible",
	[SIGPWR] = "Power failure",
	[SIGSYS] = "Unknown system call",
};

char *
strsignal(int sig)
{
	if (sig < 0) {
		return NULL;
	}
	if (sig >= NSIG) {
		return NULL;
	}
	return (char *)signal_map[sig];
}

void
psignal(int sig, const char *s)
{
	bool omit = s == NULL || strcmp(s, "") == 0;
	if (omit) {
		fprintf(stderr, "%s\n", strsignal(sig));
	} else {
		fprintf(stderr, "%s: %s\n", s, strsignal(sig));
	}
}

int
__sigsetjmp_tail(sigjmp_buf env, int ret)
{
	void *p = env->__saved_mask;
	sigprocmask(SIG_SETMASK, ret ? p : NULL, ret ? NULL : p);
	return ret;
}

__attribute__((noreturn)) void
siglongjmp(jmp_buf env, int val)
{
	longjmp(env, val);
}
