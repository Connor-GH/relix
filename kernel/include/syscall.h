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
#define SYS_alarm 13
#define SYS_uptime 14
#define SYS_open 15
#define SYS_write 16
#define SYS_mknod 17
#define SYS_unlink 18
#define SYS_link 19
#define SYS_mkdir 20
#define SYS_close 21
#define SYS_time 22
#define SYS_chmod 23
#define SYS_reboot 24
#define SYS_setgid 25
#define SYS_setuid 26
#define SYS_ptrace 27
#define SYS_symlink 28
#define SYS_readlink 29
#define SYS_lseek 30
#define SYS_fsync 31
#define SYS_writev 32
#define SYS_ioctl 33
#define SYS_mmap 34
#define SYS_munmap 35
#define SYS_signal 36
#define SYS_getcwd 37
#define SYS_sigprocmask 38
#define SYS_vfork 39
#define SYS_wait3 40
#define SYS_sigsuspend 41
#define SYS_umask 42
#define SYS_sigaction 43
#define SYS_rename 44
#define SYS_getuid 45
#define SYS_getgid 46
#define SYS_getppid 47
#define SYS_times 48
#define SYS_lstat 49
#define SYS_fchmod 50
#define SYS_access 51
#define SYS_fcntl 52
#define SYS_uname 53
#define SYSCALL_AMT 53
#ifndef __ASSEMBLER__
#include <stddef.h>
#include <sys/types.h>
__attribute__((unused)) static const char *syscall_names[SYSCALL_AMT + 1] = {
	[0] = "",
	[SYS_fork] = "fork",
	[SYS__exit] = "_exit",
	[SYS_wait] = "wait",
	[SYS_pipe] = "pipe",
	[SYS_read] = "read",
	[SYS_kill] = "kill",
	[SYS_execve] = "execve",
	[SYS_fstat] = "fstat",
	[SYS_chdir] = "chdir",
	[SYS_dup] = "dup",
	[SYS_getpid] = "getpid",
	[SYS_sbrk] = "sbrk",
	[SYS_alarm] = "alarm",
	[SYS_uptime] = "uptime",
	[SYS_open] = "open",
	[SYS_write] = "write",
	[SYS_mknod] = "mknod",
	[SYS_unlink] = "unlink",
	[SYS_link] = "link",
	[SYS_mkdir] = "mkdir",
	[SYS_close] = "close",
	[SYS_time] = "time",
	[SYS_chmod] = "chmod",
	[SYS_reboot] = "reboot",
	[SYS_setgid] = "setgid",
	[SYS_setuid] = "setuid",
	[SYS_ptrace] = "ptrace",
	[SYS_symlink] = "symlink",
	[SYS_readlink] = "readlink",
	[SYS_lseek] = "lseek",
	[SYS_fsync] = "fsync",
	[SYS_writev] = "writev",
	[SYS_ioctl] = "ioctl",
	[SYS_mmap] = "mmap",
	[SYS_munmap] = "munmap",
	[SYS_signal] = "signal",
	[SYS_getcwd] = "getcwd",
	[SYS_sigprocmask] = "sigprocmask",
	[SYS_vfork] = "vfork",
	[SYS_wait3] = "wait3",
	[SYS_sigsuspend] = "sigsuspend",
	[SYS_umask] = "umask",
	[SYS_sigaction] = "sigaction",
	[SYS_rename] = "rename",
	[SYS_getuid] = "getuid",
	[SYS_getgid] = "getgid",
	[SYS_getppid] = "getppid",
	[SYS_times] = "times",
	[SYS_lstat] = "lstat",
	[SYS_fchmod] = "fchmod",
	[SYS_access] = "access",
	[SYS_fcntl] = "fcntl",
	[SYS_uname] = "uname",
};
#endif
#if __KERNEL__ && !defined(__ASSEMBLER__)
#include <stdint.h>
/*
 * All of the generic integer type "arg" and "fetch" family of functions
 * are defined from macro generation.
 */
int argint(int, int *);
int argunsigned_int(int, unsigned int *);
int argunsigned_long(int, unsigned long *);
int arguintptr_t(int n, uintptr_t *ip);
int argintptr_t(int n, intptr_t *ip);
int argssize_t(int n, ssize_t *sp);
int argsize_t(int n, size_t *sp);
int argoff_t(int n, off_t *sp);
int argmode_t(int n, mode_t *sp);
int argdev_t(int n, dev_t *sp);
int argpid_t(int n, pid_t *sp);
int arguid_t(int n, uid_t *sp);
int arggid_t(int n, gid_t *sp);

int fetchint(uintptr_t, int *);
int fetchunsigned_int(uintptr_t, unsigned int *);
int fetchunsigned_long(uintptr_t, unsigned long *);
int fetchuintptr_t(uintptr_t addr, uintptr_t *ip);

int argptr(int, char **, int);
ssize_t argstr(int, char **);
ssize_t fetchstr(uintptr_t, char **);
void syscall(void);

// Init function for syscall/sysret.
void syscall_init(void);

/*
 * Designed for functions that return int, where
 * 0 is treated as "success", and less than zero is failure.
 * Propogates the error, if any, similar to Rust's "?" operator.
 */
#define PROPOGATE_ERR(x)   \
	{                        \
		int __ret;             \
		if ((__ret = (x)) < 0) \
			return __ret;        \
	}
#define PROPOGATE_ERR_WITH(x, expr) \
	{                                 \
		int __ret;                      \
		if ((__ret = (x)) < 0) {        \
			expr;                         \
			return __ret;                 \
		}                               \
	}
#endif
