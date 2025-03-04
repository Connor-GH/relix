#include "param.h"
#include "spinlock.h"
#include "memlayout.h"
#include "trap.h"
#include <fcntl.h>
#include <stdint.h>
#include <defs.h>
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "console.h"

#define SYSCALL_ARG_FETCH(T)                                   \
	int fetch##T(uintptr_t addr, T *ip)                          \
	{                                                            \
		struct proc *curproc = myproc();                           \
		if (addr >= curproc->sz || addr + sizeof(T) > curproc->sz) \
			return -1;                                               \
		*ip = *(T *)(addr);                                        \
		return 0;                                                  \
	}

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

SYSCALL_ARG_FETCH(int);
typedef unsigned int unsigned_int;
typedef unsigned long unsigned_long;
SYSCALL_ARG_FETCH(unsigned_int);
SYSCALL_ARG_FETCH(uintptr_t);
SYSCALL_ARG_FETCH(unsigned_long);

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
ssize_t
fetchstr(uintptr_t addr, char **pp)
{
	char *s, *ep;
	struct proc *curproc = myproc();

	if (addr >= curproc->sz)
		return -1;
	*pp = (char *)addr;
	ep = (char *)curproc->sz;
	for (s = *pp; s < ep; s++) {
		if (s != NULL && *s == 0)
			return s - *pp;
	}
	return -1;
}

// arguments passed in registers on x64
static uintptr_t
fetcharg(int n)
{
	struct proc *proc = myproc();
	switch (n) {
	case 0:
		return proc->tf->rdi;
	case 1:
		return proc->tf->rsi;
	case 2:
		return proc->tf->rdx;
	case 3:
		return proc->tf->rcx;
	case 4:
		return proc->tf->r8;
	case 5:
		return proc->tf->r9;
	default:
		__builtin_unreachable();
	}
}

#define SYSCALL_ARG_N(T)   \
	int arg##T(int n, T *ip) \
	{                        \
		*ip = fetcharg(n);     \
		return 0;              \
	}
SYSCALL_ARG_N(int);
SYSCALL_ARG_N(unsigned_int);
SYSCALL_ARG_N(unsigned_long);
SYSCALL_ARG_N(uintptr_t);
SYSCALL_ARG_N(ssize_t);
SYSCALL_ARG_N(size_t);
SYSCALL_ARG_N(off_t);

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
	uintptr_t ptr;
	struct proc *curproc = myproc();

	if (arguintptr_t(n, &ptr) < 0)
		return -1;

	if (size < 0 || ((uintptr_t)ptr >= curproc->effective_largest_sz ||
									 (uintptr_t)ptr + size > curproc->effective_largest_sz)) {
		return -1;
	}
	*pp = (char *)ptr;
	return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
ssize_t
argstr(int n, char **pp)
{
	uintptr_t addr;
	if (arguintptr_t(n, &addr) < 0)
		return -1;
	return fetchstr(addr, pp);
}

extern size_t
sys_chdir(void);
extern size_t
sys_close(void);
extern size_t
sys_dup(void);
extern size_t
sys_execve(void);
extern size_t
sys__exit(void);
extern size_t
sys_fork(void);
extern size_t
sys_fstat(void);
extern size_t
sys_getpid(void);
extern size_t
sys_kill(void);
extern size_t
sys_link(void);
extern size_t
sys_mkdir(void);
extern size_t
sys_mknod(void);
extern size_t
sys_open(void);
extern size_t
sys_pipe(void);
extern size_t
sys_read(void);
extern size_t
sys_sbrk(void);
extern size_t
sys_sleep(void);
extern size_t
sys_unlink(void);
extern size_t
sys_wait(void);
extern size_t
sys_write(void);
extern size_t
sys_uptime(void);
extern size_t
sys_date(void);
extern size_t
sys_chmod(void);
extern size_t
sys_reboot(void);
extern size_t
sys_echoout(void);
extern size_t
sys_setuid(void);
extern size_t
sys_ptrace(void);
extern size_t
sys_symlink(void);
extern size_t
sys_readlink(void);
extern size_t
sys_lseek(void);
extern size_t
sys_fsync(void);
extern size_t
sys_writev(void);
extern size_t
sys_ioctl(void);
extern size_t
sys_mmap(void);
extern size_t
sys_munmap(void);
extern size_t
sys_signal(void);
extern size_t
sys_getcwd(void);
extern size_t
sys_sigprocmask(void);
extern size_t
sys_vfork(void);
extern size_t
sys_wait3(void);
extern size_t
sys_sigsuspend(void);
extern size_t
sys_umask(void);
extern size_t
sys_sigaction(void);
extern size_t
sys_rename(void);
extern size_t
sys_getuid(void);
extern size_t
sys_getgid(void);
extern size_t
sys_getppid(void);

static size_t (*syscalls[])(void) = {
	[SYS_fork] = sys_fork,				 [SYS__exit] = sys__exit,
	[SYS_wait] = sys_wait,				 [SYS_pipe] = sys_pipe,
	[SYS_read] = sys_read,				 [SYS_kill] = sys_kill,
	[SYS_execve] = sys_execve,		 [SYS_fstat] = sys_fstat,
	[SYS_chdir] = sys_chdir,			 [SYS_dup] = sys_dup,
	[SYS_getpid] = sys_getpid,		 [SYS_sbrk] = sys_sbrk,
	[SYS_sleep] = sys_sleep,			 [SYS_uptime] = sys_uptime,
	[SYS_open] = sys_open,				 [SYS_write] = sys_write,
	[SYS_mknod] = sys_mknod,			 [SYS_unlink] = sys_unlink,
	[SYS_link] = sys_link,				 [SYS_mkdir] = sys_mkdir,
	[SYS_close] = sys_close,			 [SYS_date] = sys_date,
	[SYS_chmod] = sys_chmod,			 [SYS_reboot] = sys_reboot,
	[SYS_echoout] = sys_echoout,	 [SYS_setuid] = sys_setuid,
	[SYS_ptrace] = sys_ptrace,		 [SYS_symlink] = sys_symlink,
	[SYS_readlink] = sys_readlink, [SYS_lseek] = sys_lseek,
	[SYS_fsync] = sys_fsync,			 [SYS_writev] = sys_writev,
	[SYS_ioctl] = sys_ioctl,			 [SYS_mmap] = sys_mmap,
	[SYS_munmap] = sys_munmap,		 [SYS_signal] = sys_signal,
	[SYS_getcwd] = sys_getcwd,		 [SYS_sigprocmask] = sys_sigprocmask,
	[SYS_umask] = sys_umask,			 [SYS_sigaction] = sys_sigaction,
	[SYS_rename] = sys_rename, 		 [SYS_getuid] = sys_getuid,
	[SYS_getgid] = sys_getgid, 		 [SYS_getppid] = sys_getppid,
};

void
syscall(void)
{
	int num;
	struct proc *curproc = myproc();

	num = curproc->tf->eax;
	if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
		if (curproc->ptrace_mask_ptr[num] == 1) {
			size_t xticks, yticks;
			acquire(&tickslock);
			xticks = ticks;
			release(&tickslock);
			curproc->tf->eax = syscalls[num]();
			acquire(&tickslock);
			yticks = ticks;
			release(&tickslock);

			cprintf("pid %d: syscall %s => %ld (%lu ticks)\n", curproc->pid,
							syscall_names[num], curproc->tf->eax, yticks - xticks);
		} else {
			curproc->tf->eax = syscalls[num]();
		}
	} else {
		cprintf("%d %s: unknown sys call %d\n", curproc->pid, curproc->name, num);
		curproc->tf->eax = -1;
	}
}
