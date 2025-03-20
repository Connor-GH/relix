#include <stdint.h>
#include <date.h>
#include <errno.h>
#include <sys/reboot.h>
#include "x86.h"
#include "proc.h"
#include "syscall.h"
#include "trap.h"
#include <string.h>
#include "kernel_signal.h"
#include "drivers/lapic.h"
#include "console.h"
#include "time.h"

size_t
sys_fork(void)
{
	return fork(false);
}

size_t
sys__exit(void)
{
	int status;
	if (argint(0, &status) < 0)
		return -EINVAL;
	exit(status);
	return 0; // not reached
}

size_t
sys_wait(void)
{
	// very strange: is this correct behavior?
	int *status;
	if (argptr(0, (char **)&status, 1) < 0)
		return -EINVAL;
	return wait(status);
}

size_t
sys_kill(void)
{
	pid_t pid;
	int signal;

	if (argint(0, &pid) < 0)
		return -EINVAL;
	if (argint(1, &signal) < 0)
		return -EINVAL;
	return kill(pid, signal);
}

size_t
sys_getpid(void)
{
	return myproc()->pid;
}

size_t
sys_getppid(void)
{
	struct proc *proc = myproc();
	if (proc != NULL && proc->parent != NULL)
		return proc->parent->pid;
	else
		return 1;
}

size_t
sys_sbrk(void)
{
	uintptr_t addr;
	uintptr_t n;

	if (arguintptr_t(0, &n) < 0)
		return -EINVAL;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1; // TODO
	return addr;
}

size_t
sys_sleep(void)
{
	unsigned int n;
	time_t ticks0;

	if (argunsigned_int(0, &n) < 0)
		return -EINVAL;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (myproc()->killed) {
			release(&tickslock);
			return -ESRCH;
		}
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
size_t
sys_uptime(void)
{
	time_t xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

size_t
sys_date(void)
{
	struct rtcdate *r;
	if (argptr(0, (char **)&r, sizeof(*r)) < 0)
		return -EINVAL;
	cmostime(r);
	return 0;
}

static void __attribute__((noreturn))
poweroff(void)
{
	// get rid of init
	kill(0, SIGKILL);
	kill(1, SIGKILL);
	// shut down qemu through magic acpi numbers
	outw(0xB004, 0x0 | 0x2000);
	outw(0x604, 0x0 | 0x2000);
	// get rid of "noreturn" warning
	panic("Unreachable.");
}
size_t
sys_reboot(void)
{
	int cmd;
	if (argint(0, &cmd) < 0) {
		return -EINVAL;
	}

	switch (cmd) {
	case RB_POWER_OFF:
		kill(1, SIGKILL);
		cprintf("Shutting down.\n");
		poweroff();
		/* unreachable */
		return 0;
		break;
	case RB_HALT:
		kill(1, SIGKILL);
		cprintf("System Halted.\n");
		cli();
		hlt();
		/* unreachable */
		return 0;
		break;
	default:
		return -EINVAL;
	}
}

size_t
sys_setuid(void)
{
	// cannot setuid if not root
	if (myproc()->cred.uid != 0)
		return -EPERM;
	uid_t uid;
	if (argint(0, &uid) < 0)
		return -EINVAL;
	struct cred cred;
	cred.uid = uid;
	myproc()->cred = cred;
	return 0;
}

size_t
sys_getuid(void)
{
	return myproc()->cred.uid;
}

size_t
sys_getgid(void)
{
	return myproc()->cred.gid;
}


size_t
sys_ptrace(void)
{
	char *trace_ptr;
	if (argptr(0, &trace_ptr, SYSCALL_AMT) < 0)
		return -EINVAL;
	memmove(myproc()->ptrace_mask_ptr, trace_ptr, SYSCALL_AMT);
	return 0;
}

size_t
sys_signal(void)
{
	int signum;
	sighandler_t handler;
	if (argint(0, &signum) < 0)
		return -EINVAL;
	if (argptr(1, (char **)&handler, sizeof(*handler)) < 0)
		return -EINVAL;
	return (size_t)kernel_attach_signal(signum, handler);
}

size_t
sys_sigprocmask(void)
{
	return -ENOSYS;
}

// TODO implement properly
size_t
sys_vfork(void)
{
	return fork(true);
}

size_t
sys_wait3(void)
{
	return -ENOSYS;
}

size_t
sys_sigsuspend(void)
{
	return -ENOSYS;
}
size_t
sys_sigaction(void)
{
	return -ENOSYS;
}

size_t
sys_times(void)
{
	struct tms *tms;
	if (argptr(0, (char **)&tms, sizeof(struct tms *)) < 0)
		return -EFAULT;

	return 1;
}
