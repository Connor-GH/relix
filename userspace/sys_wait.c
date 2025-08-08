#include "libc_syscalls.h"
#include <sys/syscall.h>
#include <sys/wait.h>

pid_t
wait(int *status)
{
	return waitpid((pid_t)-1, status, 0);
}

pid_t
waitpid(pid_t pid, int *status, int options)
{
	return __syscall_ret(__syscall3(SYS_waitpid, pid, (long)status, options));
}
