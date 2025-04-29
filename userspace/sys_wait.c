#include <sys/wait.h>
#include <sys/syscall.h>
#include "libc_syscalls.h"

pid_t
wait3(int *status, int options, struct rusage *rusage)
{
	return __syscall_ret(__syscall3(SYS_wait3, (long)status, options, (long)rusage));
}
