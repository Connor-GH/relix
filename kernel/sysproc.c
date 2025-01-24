#include <stdint.h>
#include <date.h>
#include <errno.h>
#include <sys/reboot.h>
#include "x86.h"
#include "proc.h"
#include "syscall.h"
#include "trap.h"
#include "kernel_string.h"
#include "drivers/lapic.h"
#include "console.h"

int
sys_fork(void)
{
	return fork();
}

int
sys__exit(void)
{
	int status;
	if (argint(0, &status) < 0)
		return -EINVAL;
	exit(status);
	return 0; // not reached
}

int
sys_wait(void)
{
	// very strange: is this correct behavior?
	int *status;
	if (argptr(0, (char **)&status, 1) < 0)
		return -EINVAL;
	return wait(status);
}

int
sys_kill(void)
{
	int pid;

	if (argint(0, &pid) < 0)
		return -EINVAL;
	return kill(pid);
}

int
sys_getpid(void)
{
	return myproc()->pid;
}

int
sys_sbrk(void)
{
	uintptr_t addr;
	uintptr_t n;

	if (arguintptr(0, &n) < 0)
		return -EINVAL;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1; // TODO
	return addr;
}

int
sys_sleep(void)
{
	int n;
	uint32_t ticks0;

	if (argint(0, &n) < 0)
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
int
sys_uptime(void)
{
	uint32_t xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

int
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
	// sh is technically pid 0
	kill(0);
	kill(1);
	// shut down qemu through magic acpi numbers
	outw(0xB004, 0x0 | 0x2000);
	outw(0x604, 0x0 | 0x2000);
	exit(0); // get rid of "noreturn" warning
}
int
sys_reboot(void)
{
	int cmd;
	if (argint(0, &cmd) < 0) {
		return -EINVAL;
	}

	switch (cmd) {
	case RB_POWER_OFF:
		kill(1);
		cprintf("Shutting down.\n");
		poweroff();
		/* unreachable */
		return 0;
		break;
	case RB_HALT:
		kill(1);
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

int
sys_setuid(void)
{
	// cannot setuid if not root
	if (myproc()->cred.uid != 0)
		return -EPERM;
	int uid;
	if (argint(0, &uid) < 0)
		return -EINVAL;
	struct cred cred;
	cred.uid = uid;
	myproc()->cred = cred;
	return 0;
}

int
sys_strace(void)
{
	char *trace_ptr;
	if (argptr(0, &trace_ptr, SYSCALL_AMT) < 0)
		return -EINVAL;
	memmove(myproc()->strace_mask_ptr, trace_ptr, SYSCALL_AMT);
	return 0;
}
