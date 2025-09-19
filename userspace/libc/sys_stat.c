/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

int
mknodat(int dirfd, const char *pathname, mode_t mode, dev_t device)
{
	return __syscall_ret(
		__syscall4(SYS_mknodat, dirfd, (long)pathname, mode, device));
}

int
mknod(const char *pathname, mode_t mode, dev_t device)
{
	return mknodat(AT_FDCWD, pathname, mode, device);
}

int
mkdirat(int dirfd, const char *pathname, mode_t mode)
{
	return __syscall_ret(__syscall3(SYS_mkdirat, dirfd, (long)pathname, mode));
}

int
mkdir(const char *pathname, mode_t mode)
{
	return mkdirat(AT_FDCWD, pathname, mode);
}

int
fstatat(int dirfd, const char *restrict pathname, struct stat *restrict statbuf,
        int flags)
{
	return __syscall_ret(
		__syscall4(SYS_fstatat, dirfd, (long)pathname, (long)statbuf, flags));
}

int
stat(const char *restrict pathname, struct stat *restrict statbuf)
{
	return fstatat(AT_FDCWD, pathname, statbuf, 0);
}

int
lstat(const char *restrict pathname, struct stat *restrict statbuf)
{
	return fstatat(AT_FDCWD, pathname, statbuf, AT_SYMLINK_NOFOLLOW);
}

int
fstat(int fd, struct stat *restrict statbuf)
{
	return __syscall_ret(__syscall2(SYS_fstat, fd, (long)statbuf));
}

int
fchmodat(int fd, const char *pathname, mode_t mode, int flag)
{
	return __syscall_ret(
		__syscall4(SYS_fchmodat, fd, (long)pathname, mode, flag));
}

int
chmod(const char *pathname, mode_t mode)
{
	return fchmodat(AT_FDCWD, pathname, mode, 0);
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

int
mkfifoat(int dirfd, const char *pathname, mode_t mode)
{
	return mknodat(dirfd, pathname, mode | S_IFIFO, 0);
}
