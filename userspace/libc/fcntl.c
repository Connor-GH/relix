/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/types.h>

int
open(const char *pathname, int flags, ...)
{
	mode_t mode;
	va_list listp;
	if ((flags & O_CREAT) == O_CREAT || (flags & O_TMPFILE) == O_TMPFILE) {
		va_start(listp, flags);
		mode = va_arg(listp, mode_t);
		va_end(listp);
	} else {
		mode = 0;
	}
	return __syscall_ret(
		__syscall4(SYS_openat, (long)AT_FDCWD, (long)pathname, flags, mode));
}

int
creat(const char *file, mode_t mode)
{
	return open(file, O_CREAT | O_WRONLY | O_TRUNC, mode);
}

int
fcntl(int fd, int cmd, ...)
{
	void *arg;
	va_list listp;
	switch (cmd) {
	case F_SETFD:
	case F_SETFL:
	case F_SETLK:
	case F_SETLKW:
	case F_SETOWN:
		va_start(listp, cmd);
		arg = va_arg(listp, void *);
		va_end(listp);
		break;
	default:
		arg = NULL;
		break;
	}
	return __syscall_ret(__syscall3(SYS_fcntl, fd, cmd, (long)arg));
}
