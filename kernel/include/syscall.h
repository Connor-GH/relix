#pragma once
// System call numbers
// cannot be enums because they communicate with assembly.
#define SYS_fork 1
#define SYS_exit 2
#define SYS_wait 3
#define SYS_pipe 4
#define SYS_read 5
#define SYS_kill 6
#define SYS_exec 7
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
#define SYSCALL_AMT 27
#ifndef __ASSEMBLER__
__attribute__((unused)) static const char *syscall_names[SYSCALL_AMT + 1] = {
	[SYS_fork] = "fork",			 [SYS_exit] = "exit",			[SYS_wait] = "wait",
	[SYS_pipe] = "pipe",			 [SYS_read] = "read",			[SYS_kill] = "kill",
	[SYS_exec] = "exec",			 [SYS_fstat] = "fstat",		[SYS_chdir] = "chdir",
	[SYS_dup] = "dup",				 [SYS_getpid] = "getpid", [SYS_sbrk] = "sbrk",
	[SYS_sleep] = "sleep",		 [SYS_uptime] = "uptime", [SYS_open] = "open",
	[SYS_write] = "write",		 [SYS_mknod] = "mknod",		[SYS_unlink] = "unlink",
	[SYS_link] = "link",			 [SYS_mkdir] = "mkdir",		[SYS_close] = "close",
	[SYS_date] = "date",			 [SYS_chmod] = "chmod",		[SYS_reboot] = "reboot",
	[SYS_echoout] = "echoout", [SYS_setuid] = "setuid", [SYS_strace] = "strace",
};
#endif
#if defined(__KERNEL__) && !defined(__ASSEMBLER__)
#include <types.h>
int
argint(int, int *);
int
argptr(int, char **, int);
int
argstr(int, char **);
int
fetchint(uint, int *);
int
fetchstr(uint, char **);
void
syscall(void);
#endif
