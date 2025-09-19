/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <sys/ptrace.h>
#include <sys/syscall.h>

int
ptrace(char mask[static SYSCALL_AMT])
{
	return __syscall_ret(__syscall1(SYS_ptrace, (long)mask));
}
