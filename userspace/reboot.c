#include <sys/reboot.h>
#include <sys/syscall.h>
#include "libc_syscalls.h"

int
reboot(int op)
{
	return __syscall1(SYS_reboot, op);
}
