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
		neg %eax; \
		movl %eax, errno; \
		movl $-1, %eax; \
	ok_ ## name: \
		ret
  .globl name; \


#define SYSCALL_PRIVATE(name) \
  .globl __ ## name; \
  __ ## name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    cmpl $0, %eax; /* did we return successfully? */ \
		jge ok_ ## __ ## name; \
		cmpl $-MAX_ERRNO, %eax; /* do we exceed the max errno? */ \
		jl ok_ ## __ ## name; \
		neg %eax; \
		movl %eax, errno; \
		movl $-1, %eax; \
	ok_ ## __ ## name: \
		ret

SYSCALL(fork)
SYSCALL(_exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(execve)
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
SYSCALL(alarm)
SYSCALL(uptime)
SYSCALL(time)
SYSCALL(chmod)
SYSCALL(reboot)
SYSCALL(setgid)
SYSCALL(setuid)
SYSCALL(ptrace)
SYSCALL(symlink)
SYSCALL(readlink)
SYSCALL(lseek)
SYSCALL(fsync)
SYSCALL(writev)
SYSCALL(ioctl)
SYSCALL(mmap)
SYSCALL(munmap)
SYSCALL(signal)
SYSCALL_PRIVATE(getcwd)
SYSCALL(sigprocmask) # TODO
SYSCALL_PRIVATE(vfork) # TODO
SYSCALL(wait3) # TODO
SYSCALL(umask)
SYSCALL(sigsuspend) # TODO
SYSCALL(sigaction) # TODO
SYSCALL(rename)
SYSCALL(getuid)
SYSCALL(getgid)
SYSCALL(getppid)
SYSCALL(times)
SYSCALL(stat)
