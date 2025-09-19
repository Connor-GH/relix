/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "ext.h"
#include "libc_syscalls.h"
#include <sys/syscall.h>

time_t
uptime(void)
{
	return __syscall0(SYS_uptime);
}
