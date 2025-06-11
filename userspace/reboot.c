#include "libc_syscalls.h"
#include <sys/reboot.h>
#include <sys/syscall.h>

int
reboot(int op)
{
	return __syscall1(SYS_reboot, op);
}
