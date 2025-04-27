#include <sys/uio.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "libc_syscalls.h"

ssize_t
writev(int fd, const struct iovec *iov, int iovcnt)
{
	return __syscall3(SYS_writev, fd, (long)iov, iovcnt);
}
