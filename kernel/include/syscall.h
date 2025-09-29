#pragma once
// System call numbers
// cannot be enums because they communicate with assembly.
#define SYS_fork 1
#define SYS__exit 2
#define SYS_waitpid 3
#define SYS_pipe2 4
#define SYS_read 5
#define SYS_kill 6
#define SYS_execve 7
#define SYS_fstat 8
#define SYS_chdir 9
#define SYS_dup3 10
#define SYS_getpid 11
#define SYS_sbrk 12
#define SYS_alarm 13
#define SYS_uptime 14
#define SYS_openat 15
#define SYS_write 16
#define SYS_mknodat 17
#define SYS_unlinkat 18
#define SYS_linkat 19
#define SYS_mkdirat 20
#define SYS_close 21
#define SYS_clock_gettime 22
#define SYS_fchmodat 23
#define SYS_reboot 24
#define SYS_setgid 25
#define SYS_setuid 26
#define SYS_ptrace 27
#define SYS_symlinkat 28
#define SYS_readlinkat 29
#define SYS_lseek 30
#define SYS_fsync 31
#define SYS_writev 32
#define SYS_ioctl 33
#define SYS_mmap 34
#define SYS_munmap 35
#define SYS_signal 36
#define SYS_getcwd 37
#define SYS_sigprocmask 38
#define SYS_getpgid 39
#define SYS_setpgid 40
#define SYS_sigsuspend 41
#define SYS_umask 42
#define SYS_sigaction 43
#define SYS_renameat 44
#define SYS_getuid 45
#define SYS_getgid 46
#define SYS_getppid 47
#define SYS_times 48
#define SYS_fstatat 49
#define SYS_fchmod 50
#define SYS_faccessat 51
#define SYS_fcntl 52
#define SYS_uname 53
#define SYS_getdents 54
#define SYS_nanosleep 55
#define SYSCALL_AMT 55
#ifndef __ASSEMBLER__
#include <stddef.h>
#include <sys/types.h>
__attribute__((unused)) static const char *syscall_names[SYSCALL_AMT + 1] = {
	[0] = "",
	[SYS_fork] = "fork",
	[SYS__exit] = "_exit",
	[SYS_waitpid] = "waitpid",
	[SYS_pipe2] = "pipe2",
	[SYS_read] = "read",
	[SYS_kill] = "kill",
	[SYS_execve] = "execve",
	[SYS_fstat] = "fstat",
	[SYS_chdir] = "chdir",
	[SYS_dup3] = "dup3",
	[SYS_getpid] = "getpid",
	[SYS_sbrk] = "sbrk",
	[SYS_alarm] = "alarm",
	[SYS_uptime] = "uptime",
	[SYS_openat] = "openat",
	[SYS_write] = "write",
	[SYS_mknodat] = "mknodat",
	[SYS_unlinkat] = "unlinkat",
	[SYS_linkat] = "linkat",
	[SYS_mkdirat] = "mkdirat",
	[SYS_close] = "close",
	[SYS_clock_gettime] = "clock_gettime",
	[SYS_fchmodat] = "fchmodat",
	[SYS_reboot] = "reboot",
	[SYS_setgid] = "setgid",
	[SYS_setuid] = "setuid",
	[SYS_ptrace] = "ptrace",
	[SYS_symlinkat] = "symlinkat",
	[SYS_readlinkat] = "readlinkat",
	[SYS_lseek] = "lseek",
	[SYS_fsync] = "fsync",
	[SYS_writev] = "writev",
	[SYS_ioctl] = "ioctl",
	[SYS_mmap] = "mmap",
	[SYS_munmap] = "munmap",
	[SYS_signal] = "signal",
	[SYS_getcwd] = "getcwd",
	[SYS_sigprocmask] = "sigprocmask",
	[SYS_getpgid] = "getpgid",
	[SYS_setpgid] = "setpgid",
	[SYS_sigsuspend] = "sigsuspend",
	[SYS_umask] = "umask",
	[SYS_sigaction] = "sigaction",
	[SYS_renameat] = "renameat",
	[SYS_getuid] = "getuid",
	[SYS_getgid] = "getgid",
	[SYS_getppid] = "getppid",
	[SYS_times] = "times",
	[SYS_fstatat] = "fstatat",
	[SYS_fchmod] = "fchmod",
	[SYS_faccessat] = "faccessat",
	[SYS_fcntl] = "fcntl",
	[SYS_uname] = "uname",
	[SYS_getdents] = "getdents",
	[SYS_nanosleep] = "nanosleep",
};
#endif
#if __RELIX_KERNEL__ && !defined(__ASSEMBLER__)
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
int argclockid_t(int n, clockid_t *cid);

int fetchint(uintptr_t, int *);
int fetchunsigned_int(uintptr_t, unsigned int *);
int fetchunsigned_long(uintptr_t, unsigned long *);
int fetchuintptr_t(uintptr_t addr, uintptr_t *ip);

int argptr(int, char **, int);
ssize_t argstr(int, char **);
ssize_t fetchstr(uintptr_t, char **);

struct cpu;
// Init function for syscall/sysret.
void syscall_init(struct cpu *c);

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
