#include "libc_syscalls.h"
#include <sys/mman.h>
#include <sys/syscall.h>

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	return (void *)__syscall_ret(
		__syscall6(SYS_mmap, (long)addr, length, prot, flags, fd, offset));
}

int
munmap(void *addr, size_t length)
{
	return __syscall_ret(__syscall2(SYS_munmap, (long)addr, length));
}
