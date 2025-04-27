#include <sys/mman.h>
#include <sys/syscall.h>
#include "libc_syscalls.h"

void *
mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	return (void *)__syscall6(SYS_mmap, (long)addr, length, prot, flags, fd, offset);
}

int
munmap(void *addr, size_t length)
{
	return __syscall2(SYS_munmap, (long)addr, length);
}
