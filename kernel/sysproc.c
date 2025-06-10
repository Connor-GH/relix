#include <stdint.h>
#include <errno.h>
#include <sys/reboot.h>
#include <sys/times.h>
#include <time.h>
#include <string.h>
#include "x86.h"
#include "proc.h"
#include "syscall.h"
#include "trap.h"
#include "kernel_signal.h"
#include "drivers/lapic.h"
#include "console.h"

size_t
sys_fork(void)
{
	return fork(false);
}

size_t
sys__exit(void)
{
	int status;
	PROPOGATE_ERR(argint(0, &status));

	exit(status);
	return 0; // not reached
}

size_t
sys_wait(void)
{
	int *status;
	PROPOGATE_ERR(argptr(0, (char **)&status, sizeof(*status)));

	return wait(status);
}

size_t
sys_kill(void)
{
	pid_t pid;
	int signal;

	PROPOGATE_ERR(argpid_t(0, &pid));
	PROPOGATE_ERR(argint(1, &signal));

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
	if (proc != NULL && proc->parent != NULL) {
		return proc->parent->pid;
	} else {
		// If parent has been killed, use init (PID 1).
		return 1;
	}
}

size_t
sys_sbrk(void)
{
	uintptr_t addr;
	intptr_t n;

	PROPOGATE_ERR(argintptr_t(0, &n));

	addr = myproc()->sz;
	PROPOGATE_ERR(growproc(n));
	return addr;
}

size_t
sys_alarm(void)
{
	unsigned int n;
	time_t ticks0;

	PROPOGATE_ERR(argunsigned_int(0, &n));

	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (myproc()->killed) {
			release(&tickslock);
			return -ESRCH;
		}
		sleep(&ticks, &tickslock);
	}
	kill(myproc()->pid, SIGALRM);
	release(&tickslock);
	return 0;
}

// return how many clock tick interrupts have occurred
// since start.
size_t
sys_uptime(void)
{
	time_t xticks;

	xticks = ticks;
	return xticks;
}

size_t
sys_time(void)
{
	time_t *time;
	PROPOGATE_ERR(argptr(0, (char **)&time, sizeof(*time)));

	time_t cur_time = rtc_now();
	if (time != NULL) {
		*time = cur_time;
	}
	return cur_time;
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
	PROPOGATE_ERR(argint(0, &cmd));

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
sys_setgid(void)
{
	// cannot setuid if not root
	if (myproc()->cred.gid != 0) {
		return -EPERM;
	}
	gid_t gid;
	PROPOGATE_ERR(arggid_t(0, &gid));

	myproc()->cred.gid = gid;
	return 0;
}

size_t
sys_setuid(void)
{
	// cannot setuid if not root
	if (myproc()->cred.uid != 0) {
		return -EPERM;
	}
	uid_t uid;
	PROPOGATE_ERR(arguid_t(0, &uid));

	myproc()->cred.uid = uid;
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
	PROPOGATE_ERR(argptr(0, &trace_ptr, SYSCALL_AMT));

	memmove(myproc()->ptrace_mask_ptr, trace_ptr, SYSCALL_AMT);
	return 0;
}

size_t
sys_signal(void)
{
	int signum;
	sighandler_t handler;
	PROPOGATE_ERR(argint(0, &signum));
	PROPOGATE_ERR(argptr(1, (char **)&handler, sizeof(*handler)));

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
	PROPOGATE_ERR(argptr(0, (char **)&tms, sizeof(*tms)));

	return -ENOSYS;
}
