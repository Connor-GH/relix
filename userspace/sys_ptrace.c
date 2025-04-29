#include <sys/ptrace.h>
#include <sys/syscall.h>
#include "libc_syscalls.h"

int
ptrace(char mask[static SYSCALL_AMT])
{
	return __syscall_ret(__syscall1(SYS_ptrace, (long)mask));
}
