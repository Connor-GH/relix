#include "libc_syscalls.h"
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

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
	errno = 0;
	char buf[PATH_MAX] = {};
	// The only EINVAL readlink does is negative size, or file not
	// being a symbolic link. Guarantee the former case can't happen,
	// so we can depend on the latter for auto dereferencing.
	_Static_assert(sizeof(buf) >= 0, "");
	ssize_t ret = readlink(pathname, buf, sizeof(buf));
	// Failed with EINVAL, meaning the file is not a symbolic link.
	// We will continue on using the regular pathname then.
	if (errno == EINVAL) {
		return __syscall_ret(__syscall2(SYS_lstat, (long)pathname, (long)statbuf));
	} else if (ret < 0) {
		return __syscall_ret(ret);
	} else {
		return __syscall_ret(__syscall2(SYS_lstat, (long)buf, (long)statbuf));
	}
}

int
lstat(const char *restrict pathname, struct stat *restrict statbuf)
{
	return __syscall_ret(__syscall2(SYS_lstat, (long)pathname, (long)statbuf));
}

int
fstat(int fd, struct stat *restrict statbuf)
{
	return __syscall_ret(__syscall2(SYS_fstat, fd, (long)statbuf));
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
