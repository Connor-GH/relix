#include "libc_syscalls.h"
#include <sys/stat.h>
#include <sys/syscall.h>

int
mknod(const char *pathname, mode_t mode, dev_t device)
{
	return __syscall_ret(__syscall3(SYS_mknod, (long)pathname, mode, device));
}

int
mkdir(const char *pathname, mode_t mode)
{
	return __syscall_ret(__syscall2(SYS_mkdir, (long)pathname, mode));
}

int
stat(const char *restrict pathname, struct stat *restrict statbuf)
{
	return __syscall_ret(__syscall2(SYS_stat, (long)pathname, (long)statbuf));
}

int
fstat(int fd, struct stat *restrict statbuf)
{
	return __syscall_ret(__syscall2(SYS_fstat, fd, (long)statbuf));
}

// This should probably be moved into the kernel, because
// we have more symbolic link information there. It might
// be possible, however, to keep this in userspace if we
// can get enough information about files without too many
// syscalls.
int
lstat(const char *restrict n, struct stat *restrict st)
{
	return stat(n, st);
}

int
chmod(const char *pathname, mode_t mode)
{
	return __syscall_ret(__syscall2(SYS_chmod, (long)pathname, mode));
}

int
fchmod(int fd, mode_t mode)
{
	return __syscall_ret(__syscall2(SYS_fchmod, fd, mode));
}

mode_t
umask(mode_t mask)
{
	return __syscall_ret(__syscall1(SYS_umask, mask));
}

int
mkfifo(const char *pathname, mode_t mode)
{
	return mknod(pathname, mode | S_IFIFO, 0);
}
