/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/wait.h>

pid_t
wait(int *status)
{
	return waitpid((pid_t)-1, status, 0);
}

pid_t
wait3(int *status, int options, struct rusage *rusage)
{
	if (rusage == NULL) {
		return waitpid(-1, status, options);
	} else {
		errno = ENOSYS;
		return -1;
	}
}

pid_t
wait4(pid_t pid, int *status, int options, struct rusage *rusage)
{
	if (rusage == NULL) {
		return waitpid(pid, status, options);
	} else {
		errno = ENOSYS;
		return -1;
	}
}

pid_t
waitpid(pid_t pid, int *status, int options)
{
	return __syscall_ret(__syscall3(SYS_waitpid, pid, (long)status, options));
}
