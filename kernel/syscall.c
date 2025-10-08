#include "syscall.h"
#include "console.h"
#include "mmu.h"
#include "msr.h"
#include "proc.h"
#include "spinlock.h"
#include "trap.h"
#include "x86.h"
#include <defs.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

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

static void syscall(struct proc *curproc);
// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
ssize_t
fetchstr(uintptr_t addr, char **pp)
{
	struct proc *curproc = myproc();

	if (addr >= curproc->sz) {
		return -EFAULT;
	}
	*pp = (char *)addr;
	char *ep = (char *)curproc->sz;
	for (char *s = *pp; s < ep; s++) {
		if (s != NULL && *s == 0) {
			return s - *pp;
		}
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
SYSCALL_ARG_N(clockid_t);

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int
argptr(int n, char **pp, int size)
{
	uintptr_t ptr;
	struct proc *curproc = myproc();

	PROPOGATE_ERR(arguintptr_t(n, &ptr));

	if (size < 0 ||
	    ((uintptr_t)ptr >= curproc->sz || (uintptr_t)ptr + size > curproc->sz)) {
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

extern size_t sys_chdir(void);
extern size_t sys_close(void);
extern size_t sys_dup3(void);
extern size_t sys_execve(void);
extern size_t sys__exit(void);
extern size_t sys_fork(void);
extern size_t sys_fstat(void);
extern size_t sys_getpid(void);
extern size_t sys_kill(void);
extern size_t sys_linkat(void);
extern size_t sys_mkdirat(void);
extern size_t sys_mknodat(void);
extern size_t sys_openat(void);
extern size_t sys_pipe2(void);
extern size_t sys_read(void);
extern size_t sys_sbrk(void);
extern size_t sys_alarm(void);
extern size_t sys_unlinkat(void);
extern size_t sys_waitpid(void);
extern size_t sys_write(void);
extern size_t sys_uptime(void);
extern size_t sys_clock_gettime(void);
extern size_t sys_fchmodat(void);
extern size_t sys_reboot(void);
extern size_t sys_setgid(void);
extern size_t sys_setuid(void);
extern size_t sys_getpgid(void);
extern size_t sys_setpgid(void);
extern size_t sys_ptrace(void);
extern size_t sys_symlinkat(void);
extern size_t sys_readlinkat(void);
extern size_t sys_lseek(void);
extern size_t sys_fsync(void);
extern size_t sys_writev(void);
extern size_t sys_ioctl(void);
extern size_t sys_mmap(void);
extern size_t sys_munmap(void);
extern size_t sys_signal(void);
extern size_t sys_getcwd(void);
extern size_t sys_sigprocmask(void);
extern size_t sys_sigsuspend(void);
extern size_t sys_umask(void);
extern size_t sys_sigaction(void);
extern size_t sys_renameat(void);
extern size_t sys_getuid(void);
extern size_t sys_getgid(void);
extern size_t sys_getppid(void);
extern size_t sys_times(void);
extern size_t sys_fstatat(void);
extern size_t sys_fchmod(void);
extern size_t sys_faccessat(void);
extern size_t sys_fcntl(void);
extern size_t sys_uname(void);
extern size_t sys_getdents(void);
extern size_t sys_nanosleep(void);
extern size_t sys_geteuid(void);
extern size_t sys_getegid(void);
extern size_t sys_seteuid(void);
extern size_t sys_setegid(void);
extern size_t sys_getgroups(void);

static size_t
unknown_syscall(void)
{
	return -1;
}

static size_t (*syscalls[])(void) = {
	[SYS_fork] = sys_fork,
	[SYS__exit] = sys__exit,
	[SYS_waitpid] = sys_waitpid,
	[SYS_pipe2] = sys_pipe2,
	[SYS_read] = sys_read,
	[SYS_kill] = sys_kill,
	[SYS_execve] = sys_execve,
	[SYS_fstat] = sys_fstat,
	[SYS_chdir] = sys_chdir,
	[SYS_dup3] = sys_dup3,
	[SYS_getpid] = sys_getpid,
	[SYS_sbrk] = sys_sbrk,
	[SYS_alarm] = sys_alarm,
	[SYS_uptime] = sys_uptime,
	[SYS_openat] = sys_openat,
	[SYS_write] = sys_write,
	[SYS_mknodat] = sys_mknodat,
	[SYS_unlinkat] = sys_unlinkat,
	[SYS_linkat] = sys_linkat,
	[SYS_mkdirat] = sys_mkdirat,
	[SYS_close] = sys_close,
	[SYS_clock_gettime] = sys_clock_gettime,
	[SYS_fchmodat] = sys_fchmodat,
	[SYS_reboot] = sys_reboot,
	[SYS_setgid] = sys_setgid,
	[SYS_setuid] = sys_setuid,
	[SYS_ptrace] = sys_ptrace,
	[SYS_symlinkat] = sys_symlinkat,
	[SYS_readlinkat] = sys_readlinkat,
	[SYS_lseek] = sys_lseek,
	[SYS_fsync] = sys_fsync,
	[SYS_writev] = sys_writev,
	[SYS_ioctl] = sys_ioctl,
	[SYS_mmap] = sys_mmap,
	[SYS_munmap] = sys_munmap,
	[SYS_signal] = sys_signal,
	[SYS_getcwd] = sys_getcwd,
	[SYS_sigprocmask] = sys_sigprocmask,
	[SYS_getpgid] = sys_getpgid,
	[SYS_setpgid] = sys_setpgid,
	[SYS_sigsuspend] = sys_sigsuspend,
	[SYS_umask] = sys_umask,
	[SYS_sigaction] = sys_sigaction,
	[SYS_renameat] = sys_renameat,
	[SYS_getuid] = sys_getuid,
	[SYS_getgid] = sys_getgid,
	[SYS_getppid] = sys_getppid,
	[SYS_times] = sys_times,
	[SYS_fstatat] = sys_fstatat,
	[SYS_fchmod] = sys_fchmod,
	[SYS_faccessat] = sys_faccessat,
	[SYS_fcntl] = sys_fcntl,
	[SYS_uname] = sys_uname,
	[SYS_getdents] = sys_getdents,
	[SYS_nanosleep] = sys_nanosleep,
	[SYS_geteuid] = sys_geteuid,
	[SYS_getegid] = sys_getegid,
	[SYS_seteuid] = sys_seteuid,
	[SYS_setegid] = sys_setegid,
	[SYS_getgroups] = sys_getgroups,
};

__noreturn static void syscall_do(void);

__attribute__((noinline)) void
syscall_init(struct cpu *c)
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

	write_gs(SEG_KDATA);
	wrmsr(MSR_KERNEL_GS_BASE, (uint64_t)c);
	wrmsr(MSR_GS_BASE, (uint64_t)c);
}

void syswrap(struct trapframe *tf);

__noreturn __attribute__((naked)) static void
syscall_do(void)
{
	__asm__ __volatile__(

		"swapgs\n" // user %gs -> kernel %gs

		"mov %%rsp, %%gs:%c[user_stack]\n"
		"mov %%gs:%c[kernel_stack], %%rsp\n"

		"pushq $0x1b\n" // ss

		"pushq %%gs:%c[user_stack]\n" // user rsp.
		"sti\n"

		"pushq %%r11\n" // rflags
		"pushq $0x23\n" // cs
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

		// We used %gs for all we need it for (CPU-local data).
	  // Swap it back.
		"swapgs\n"
		"sysretq\n"
		:
		/* clang-format off */
		: [user_stack] "i"(offsetof(struct cpu, user_stack)),
		[kernel_stack] "i"(offsetof(struct cpu, kernel_stack)));
	/* clang-format on */
}

// INVARIANT: myproc() is not NULL.
// This should never be NULL since this is
// only called upon a syscall, which is in
// userspace. By that point, we have processes.
void
syswrap(struct trapframe *tf)
{
	struct proc *curproc = myproc();
	curproc->tf = tf;
	if (curproc->killed) {
		exit(0);
	}
	syscall(curproc);
	if (curproc->killed) {
		exit(0);
	}
}

static void
syscall(struct proc *curproc)
{
	uint64_t num;

	num = curproc->tf->rax;
	if (num > 0 && num < NELEM(syscalls) && syscalls[num] != NULL) {
		if (curproc->ptrace_mask_ptr[num] == 1) {
			size_t xticks, yticks;
			xticks = ticks;
			curproc->tf->rax = syscalls[num]();
			yticks = ticks;

			cprintf("pid %d: syscall %s => %ld (%lu ticks)", curproc->pid,
			        syscall_names[num], curproc->tf->rax, yticks - xticks);
			if ((signed)curproc->tf->rax < 0) {
				cprintf(" errno %d", -(signed)curproc->tf->rax);
			}
			cprintf("\n");
		} else {
			curproc->tf->rax = syscalls[num]();
		}
	} else {
		curproc->tf->rax = unknown_syscall();
	}
}
