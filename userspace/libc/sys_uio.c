/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>

ssize_t
writev(int fd, const struct iovec *iov, int iovcnt)
{
	return __syscall_ret(__syscall3(SYS_writev, fd, (long)iov, iovcnt));
}
