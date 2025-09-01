#pragma once
#if __KERNEL__
// Per-CPU state
#include "file.h"
#include "fs.h"
#include "kernel_signal.h"
#include "mman.h"
#include "mmu.h"
#include "param.h"
#include "spinlock.h"
#include "syscall.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

struct cred {
	uid_t uid; // User ID
	pid_t gid; // Group ID of current process
	gid_t gids[NGROUPS_MAX]; // all Group IDs the user is in
};

struct cpu {
	struct cpu *self;
	uint64_t kernel_stack;
	uint64_t user_stack;
	uint8_t apicid; // Local APIC ID
	struct context *scheduler; // swtch() here to enter scheduler
	struct taskstate64 *tss __attribute__((aligned(16))); // Used by x86 to find
	                                                      // stack for interrupt
	union {
		struct segdesc gdt[NSEGS]; // x86 global descriptor table
#if X86_64
		uint64_t gdt_bits[NSEGS];
#endif
	};
	volatile uint32_t started; // Has the CPU started?
	int ncli; // Depth of pushcli nesting.
	int intena; // Were interrupts enabled before pushcli?
	struct proc *proc; // The process running on this cpu or null
#if X86_64
	void *local;
#endif
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %rax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save rip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
#if X86_64
	uintptr_t r15;
	uintptr_t r14;
	uintptr_t r13;
	uintptr_t r12;
	uintptr_t rbx;
	uintptr_t rbp;
	uintptr_t rip;
#endif
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, STOPPED };

// Per-process state
struct proc {
	uintptr_t sz; // Size of process memory (bytes)
	uintptr_t *pgdir; // Page table
	char *kstack; // Bottom of kernel stack for this process
	enum procstate state; // Process state
	pid_t pid; // Process ID
	pid_t pgid; // Process Group ID
	int status;
	struct proc *parent; // Parent process
	struct trapframe *tf; // Trap frame for current syscall
	struct context *context; // swtch() here to run process
	void *chan; // If non-zero, sleeping on chan
	int killed; // If non-zero, have been killed
	struct file *ofile[OPEN_MAX]; // Open files
	struct inode *cwd; // Current directory
	struct cred cred; // user's credentials for the process.
	char name[16]; // Process name (debugging)
	char ptrace_mask_ptr[SYSCALL_AMT + 1]; // mask for tracing syscalls
	struct mmap_info mmap_info[NMMAP];
	size_t mmap_count;
	uintptr_t heap; // Location of the heap.
	uintptr_t heapsz;
	sighandler_t sig_handlers[__SIG_last];
	int last_signal;
	mode_t umask;
	uint8_t legacy_fpu_state[512] __attribute__((aligned(16)));
	uint8_t fpu_header[64] __attribute__((aligned(16)));
	uint8_t extended_fpu_state[0] __attribute__((aligned(16)));
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

int my_cpu_id(void);
void exit(int) __attribute__((noreturn));
pid_t fork(void);
int growproc(intptr_t);
int kill(pid_t, int);

// Slow version of mycpu() used in early boot.
struct cpu *early_init_mycpu(void);
struct cpu *mycpu(void);
struct proc *myproc(void);

void pinit(void);
void procdump(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void setproc(struct proc *);
void sleep(void *, struct spinlock *);
void userinit(void);
int waitpid(pid_t pid, int *status, int options);
void wakeup(void *);
void sleep_on_ms(time_t ms);
void yield(void);
struct proc *last_proc_ran(void);
bool is_in_group(gid_t group, struct cred *cred);
struct proc *get_process_from_pid(pid_t pid);
#endif
