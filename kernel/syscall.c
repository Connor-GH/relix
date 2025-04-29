#include "mmu.h"
#include "param.h"
#include "spinlock.h"
#include "memlayout.h"
#include "trap.h"
#include <errno.h>
#include "kernel_assert.h"
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <defs.h>
#include "proc.h"
#include "x86.h"
#include "syscall.h"
#include "console.h"
#include "msr.h"
#include <stdnoreturn.h>

#define SYSCALL_ARG_FETCH(T)                                   \
	int fetch##T(uintptr_t addr, T *ip)                          \
	{                                                            \
		struct proc *curproc = myproc();                           \
		if (addr >= curproc->sz || addr + sizeof(T) > curproc->sz) \
			return -EFAULT;                                          \
		*ip = *(T *)(addr);                                        \
		return 0;                                                  \
	}

// User code makes a system call with INT T_SYSCALL.
// System call number in %rax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %rsp points
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
		return -EFAULT;
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
		return proc->tf->r10;
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
SYSCALL_ARG_N(intptr_t);
SYSCALL_ARG_N(ssize_t);
SYSCALL_ARG_N(size_t);
SYSCALL_ARG_N(off_t);
SYSCALL_ARG_N(mode_t);
SYSCALL_ARG_N(dev_t);
SYSCALL_ARG_N(pid_t);
SYSCALL_ARG_N(uid_t);
SYSCALL_ARG_N(gid_t);

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
	uintptr_t ptr;
	struct proc *curproc = myproc();

	PROPOGATE_ERR(arguintptr_t(n, &ptr));

	if (size < 0 || ((uintptr_t)ptr >= curproc->effective_largest_sz ||
									 (uintptr_t)ptr + size > curproc->effective_largest_sz)) {
		return -EFAULT;
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
	PROPOGATE_ERR(arguintptr_t(n, &addr));

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
sys_alarm(void);
extern size_t
sys_unlink(void);
extern size_t
sys_wait(void);
extern size_t
sys_write(void);
extern size_t
sys_uptime(void);
extern size_t
sys_time(void);
extern size_t
sys_chmod(void);
extern size_t
sys_reboot(void);
extern size_t
sys_setgid(void);
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
extern size_t
sys_times(void);
extern size_t
sys_stat(void);
extern size_t
sys_fchmod(void);
extern size_t
sys_access(void);
extern size_t
sys_fcntl(void);

static size_t (*syscalls[])(void) = {
	[SYS_fork] = sys_fork,
	[SYS__exit] = sys__exit,
	[SYS_wait] = sys_wait,
	[SYS_pipe] = sys_pipe,
	[SYS_read] = sys_read,
	[SYS_kill] = sys_kill,
	[SYS_execve] = sys_execve,
	[SYS_fstat] = sys_fstat,
	[SYS_chdir] = sys_chdir,
	[SYS_dup] = sys_dup,
	[SYS_getpid] = sys_getpid,
	[SYS_sbrk] = sys_sbrk,
	[SYS_alarm] = sys_alarm,
	[SYS_uptime] = sys_uptime,
	[SYS_open] = sys_open,
	[SYS_write] = sys_write,
	[SYS_mknod] = sys_mknod,
	[SYS_unlink] = sys_unlink,
	[SYS_link] = sys_link,
	[SYS_mkdir] = sys_mkdir,
	[SYS_close] = sys_close,
	[SYS_time] = sys_time,
	[SYS_chmod] = sys_chmod,
	[SYS_reboot] = sys_reboot,
	[SYS_setgid] = sys_setgid,
	[SYS_setuid] = sys_setuid,
	[SYS_ptrace] = sys_ptrace,
	[SYS_symlink] = sys_symlink,
	[SYS_readlink] = sys_readlink,
	[SYS_lseek] = sys_lseek,
	[SYS_fsync] = sys_fsync,
	[SYS_writev] = sys_writev,
	[SYS_ioctl] = sys_ioctl,
	[SYS_mmap] = sys_mmap,
	[SYS_munmap] = sys_munmap,
	[SYS_signal] = sys_signal,
	[SYS_getcwd] = sys_getcwd,
	[SYS_sigprocmask] = sys_sigprocmask,
	[SYS_vfork] = sys_vfork,
	[SYS_wait3] = sys_wait3,
	[SYS_sigsuspend] = sys_sigsuspend,
	[SYS_umask] = sys_umask,
	[SYS_sigaction] = sys_sigaction,
	[SYS_rename] = sys_rename,
	[SYS_getuid] = sys_getuid,
	[SYS_getgid] = sys_getgid,
	[SYS_getppid] = sys_getppid,
	[SYS_times] = sys_times,
	[SYS_stat] = sys_stat,
	[SYS_fchmod] = sys_fchmod,
	[SYS_access] = sys_access,
	[SYS_fcntl] = sys_fcntl,
};

noreturn static void
syscall_do(void);

__attribute__((noinline)) void
syscall_init(void)
{
	// Tell the EFER to enable syscall/sysret.
	wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SCE);
	// Show LSTAR where the syscall function is.
	wrmsr(MSR_LSTAR, (uintptr_t)&syscall_do);
	uint64_t star = 0;
	// For the "target sysret" mode (user mode), %ss is 63..48 + 16.
	// This is because they expect your GDT to look like the following:
	// KCODE
	// KDATA
	// UCODE32
	// UDATA (previously: UDATA32 before IA-32e, which is amd64)
	// [something]
	// UCODE64
	//
	// UCODE32 + 16 == UCODE64
	// UCODE + 8 == UDATA
	//
	// Our GDT looks like the following:
	// KCODE
	// KDATA
	// UDATA
	// UCODE
	// and so we do "-16" to set the ss at UCODE,
	// and this results in a "-8" for UDATA.
	//
	// And the compatability mode uses UCODE32 + 0 == UCODE32.
	star |= ((((unsigned long)SEG_UCODE << 3UL) - 16) | DPL_USER) << 48UL;
	star |= ((((unsigned long)SEG_KCODE << 3UL)) | DPL_KERNEL) << 32UL;

	wrmsr(MSR_STAR, star);
	wrmsr(MSR_FMASK, FL_DIRECTION | FL_IF | FL_TRAP);
	wrmsr(MSR_KERNEL_GS_BASE,(uint64_t)mycpu());
}

void
syswrap(struct trapframe *tf);

noreturn __attribute__((naked))
static void
syscall_do(void)
{
	__asm__ __volatile__(
		"swapgs\n" // user %gs -> kernel %gs

		"mov %%rsp, %%gs:%c[user_stack]\n"
		"mov %%gs:%c[kernel_stack], %%rsp\n"

		"pushq $0\n" // ss

		"pushq %%gs:%c[user_stack]\n" // user rsp.
		// We used %gs for all we need it for (CPU-local data).
		// Swap it back.
		"swapgs\n"
		"sti\n"

		"pushq %%r11\n" // rflags
		"movq %%cs, %%r11\n"
		"pushq %%r11\n" // cs
		"pushq %%rcx\n" // user rip is preserved in rcx.
		"pushq $0\n" // err
		"pushq $0\n" // trapno
		"pushq %%r15\n"
		"pushq %%r14\n"
		"pushq %%r13\n"
		"pushq %%r12\n"
		"pushq $0\n" // r11 was trashed (rflags)
		"pushq %%r10\n" // r10 is caller-save
		"pushq %%r9\n" // r9 is caller-save
		"pushq %%r8\n" // r8 is caller-save and scratch
		"pushq %%rdi\n" // rdi is caller-save
		"pushq %%rsi\n" // rsi is caller-save
		"pushq %%rbp\n"
		"pushq %%rdx\n" // rdx is caller-save
		"pushq $0\n" // rcx was trashed (rip)
		"pushq %%rbx\n"
		"pushq %%rax\n"

		"xor %%rbp, %%rbp\n"
		"movq %%rsp, %%rdi\n"
		"callq syswrap\n"

		"popq %%rax\n"
		"popq %%rbx\n"
		"addq $8, %%rsp\n"
		"popq %%rdx\n"
		"popq %%rbp\n"
		"popq %%rsi\n"
		"popq %%rdi\n"
		"popq %%r8\n"
		"popq %%r9\n"
		"popq %%r10\n"
		"popq %%r11\n"
		"popq %%r12\n"
		"popq %%r13\n"
		"popq %%r14\n"
		"popq %%r15\n"

		"addq $(8 * 2), %%rsp\n" // skip trapno and err
		"popq %%rcx\n" // rip
		"addq $8, %%rsp\n" // skip cs
		"popq %%r11\n" // rflags

		"cli\n"
		"popq %%rsp\n" // rsp
		// TODO stack leak of %ss?

		"sysretq\n"
		:
		: [user_stack] "i" (offsetof(struct cpu, user_stack)),
			[kernel_stack] "i" (offsetof(struct cpu, kernel_stack))
	);
}
_Static_assert(offsetof(struct cpu, user_stack) == 16, "");
_Static_assert(offsetof(struct cpu, kernel_stack) == 8, "");

void
syswrap(struct trapframe *tf)
{
	kernel_assert(myproc() != NULL);
	myproc()->tf = tf;
	syscall();
}

void
syscall(void)
{
	int num;
	struct proc *curproc = myproc();

	num = curproc->tf->rax;
	if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
		if (curproc->ptrace_mask_ptr[num] == 1) {
			size_t xticks, yticks;
			acquire(&tickslock);
			xticks = ticks;
			release(&tickslock);
			curproc->tf->rax = syscalls[num]();
			acquire(&tickslock);
			yticks = ticks;
			release(&tickslock);

			cprintf("pid %d: syscall %s => %ld (%lu ticks)", curproc->pid,
							syscall_names[num], curproc->tf->rax, yticks - xticks);
			if ((signed)curproc->tf->rax < 0) {
				cprintf(" \"%s\"", errno_codes[-(signed)curproc->tf->rax]);
			}
			cprintf("\n");
		} else {
			curproc->tf->rax = syscalls[num]();
		}
	} else {
		cprintf("%d %s: unknown sys call %d\n", curproc->pid, curproc->name, num);
		curproc->tf->rax = -1;
	}
}
