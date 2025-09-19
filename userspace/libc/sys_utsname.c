/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include <errno.h>
#include <libc_syscalls.h>
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>

int
uname(struct utsname *utsname)
{
	return __syscall_ret(syscall(SYS_uname, (long)utsname));
	return 0;
}
