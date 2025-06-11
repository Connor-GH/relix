#include "libc_syscalls.h"
#include <errno.h>
#include <signal.h>
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
