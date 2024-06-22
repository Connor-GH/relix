#include <types.h>
#include <defs.h>
#include <date.h>
#include <sys/reboot.h>
#include "drivers/memlayout.h"
#include "x86.h"
#include "proc.h"

int
sys_fork(void)
{
	return fork();
}

int
sys_exit(void)
{
	int status;
	if (argint(0, &status) < 0)
		return -1;
	exit(status);
	return 0; // not reached
}

int
sys_wait(void)
{
	// very strange: is this correct behavior?
	int *status;
	if (argptr(0, (char **)&status, 1) < 0)
		return -1;
	return wait(status);
}

int
sys_kill(void)
{
	int pid;

	if (argint(0, &pid) < 0)
		return -1;
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
	int addr;
	int n;

	if (argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}

int
sys_sleep(void)
{
	int n;
	uint ticks0;

	if (argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n) {
		if (myproc()->killed) {
			release(&tickslock);
			return -1;
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
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

int
sys_date(void)
{
	struct rtcdate *r = (struct rtcdate *)(myproc()->tf->esp + 4 + 4 * 6);
	cmostime(r);
	return 0;
}

static void __attribute__((noreturn)) poweroff(void)
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
		return -1;
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
		return -1;
	}
}

int
sys_setuid(void)
{
	// cannot setuid if not root
	if (myproc()->cred.uid != 0)
		return -1;
	int uid;
	if (argint(0, &uid) < 0)
		return -1;
	struct cred cred;
	cred.uid = uid;
	cred.gids[0] = uid;
	myproc()->cred = cred;
	return 0;
}
