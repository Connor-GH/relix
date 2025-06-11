#include "libc_syscalls.h"
#include <sys/syscall.h>
#include <sys/wait.h>

pid_t
wait3(int *status, int options, struct rusage *rusage)
{
	return __syscall_ret(
		__syscall3(SYS_wait3, (long)status, options, (long)rusage));
}
