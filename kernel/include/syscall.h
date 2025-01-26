#pragma once
// System call numbers
// cannot be enums because they communicate with assembly.
#define SYS_fork 1
#define SYS__exit 2
#define SYS_wait 3
#define SYS_pipe 4
#define SYS_read 5
#define SYS_kill 6
#define SYS_execve 7
#define SYS_fstat 8
#define SYS_chdir 9
#define SYS_dup 10
#define SYS_getpid 11
#define SYS_sbrk 12
#define SYS_sleep 13
#define SYS_uptime 14
#define SYS_open 15
#define SYS_write 16
#define SYS_mknod 17
#define SYS_unlink 18
#define SYS_link 19
#define SYS_mkdir 20
#define SYS_close 21
#define SYS_date 22
#define SYS_chmod 23
#define SYS_reboot 24
#define SYS_echoout 25
#define SYS_setuid 26
#define SYS_strace 27
#define SYS_symlink 28
#define SYS_readlink 29
#define SYS_lseek 30
#define SYS_fsync 31
#define SYS_writev 32
#define SYSCALL_AMT 32
#ifndef __ASSEMBLER__
#include <stddef.h>
#include "types.h"
__attribute__((unused)) static const char *syscall_names[SYSCALL_AMT + 1] = {
	[SYS_fork] = "fork",				 [SYS__exit] = "_exit",
	[SYS_wait] = "wait",				 [SYS_pipe] = "pipe",
	[SYS_read] = "read",				 [SYS_kill] = "kill",
	[SYS_execve] = "execve",		 [SYS_fstat] = "fstat",
	[SYS_chdir] = "chdir",			 [SYS_dup] = "dup",
	[SYS_getpid] = "getpid",		 [SYS_sbrk] = "sbrk",
	[SYS_sleep] = "sleep",			 [SYS_uptime] = "uptime",
	[SYS_open] = "open",				 [SYS_write] = "write",
	[SYS_mknod] = "mknod",			 [SYS_unlink] = "unlink",
	[SYS_link] = "link",				 [SYS_mkdir] = "mkdir",
	[SYS_close] = "close",			 [SYS_date] = "date",
	[SYS_chmod] = "chmod",			 [SYS_reboot] = "reboot",
	[SYS_echoout] = "echoout",	 [SYS_setuid] = "setuid",
	[SYS_strace] = "strace",		 [SYS_symlink] = "symlink",
	[SYS_readlink] = "readlink", [SYS_lseek] = "lseek",
	[SYS_fsync] = "fsync",			 [SYS_writev] = "writev",
};
#endif
#if defined(__KERNEL__) && !defined(__ASSEMBLER__)
#include <stdint.h>
int
argint(int, int *);
int
argptr(int, char **, int);
int
arguintptr(int n, uintptr_t *ip);
int
argssize(int n, ssize_t *sp);
ssize_t
argstr(int, char **);
int
fetchint(uintptr_t, int *);
int
fetchuintp(uintptr_t addr, uintptr_t *ip);
ssize_t
fetchstr(uintptr_t, char **);
void
syscall(void);
#endif
