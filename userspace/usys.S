#include <sys/syscall.h>
#include "kernel/include/traps.h"
#include <errno.h>

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    cmpl $0, %eax; /* did we return successfully? */ \
		jge ok_ ## name; \
		cmpl $-MAX_ERRNO, %eax; /* do we exceed the max errno? */ \
		jl ok_ ## name; \
		movl $0, errno; \
		subl %eax, errno; \
		movl $-1, %eax; \
	ok_ ## name: \
		ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(date)
SYSCALL(chmod)
SYSCALL(reboot)
SYSCALL(echoout)
SYSCALL(setuid)
SYSCALL(strace)
