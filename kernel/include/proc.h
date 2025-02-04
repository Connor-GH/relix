#pragma once
// Per-CPU state
#include "fs.h"
#include "mman.h"
#include "param.h"
#include "spinlock.h"
#include "syscall.h"
#include "types.h"
#include <stdint.h>
#include "../drivers/mmu.h"
#include "../include/file.h"

struct groups {
	char *gr_name;
	char *gr_passwd;
	gid_t gr_gid;
	char **gr_mem;
};

struct cred {
	uid_t uid;
	gid_t gid;
	struct groups groups[MAXGROUPS];
};

struct cpu {
	uint8_t apicid; // Local APIC ID
	struct context *scheduler; // swtch() here to enter scheduler
	struct taskstate ts; // Used by x86 to find stack for interrupt
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
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
#if X86_64
	uintptr_t r15;
	uintptr_t r14;
	uintptr_t r13;
	uintptr_t r12;
	uintptr_t r11;
	uintptr_t rbx;
	uintptr_t ebp;
	uintptr_t eip;
#else
	uintptr_t edi;
	uintptr_t esi;
	uintptr_t ebx;
	uintptr_t ebp;
	uintptr_t eip;
#endif
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
	uintptr_t sz; // Size of process memory (bytes)
	uintptr_t *pgdir; // Page table
	char *kstack; // Bottom of kernel stack for this process
	enum procstate state; // Process state
	pid_t pid; // Process ID
	int status;
	struct proc *parent; // Parent process
	struct trapframe *tf; // Trap frame for current syscall
	struct context *context; // swtch() here to run process
	void *chan; // If non-zero, sleeping on chan
	int killed; // If non-zero, have been killed
	struct file *ofile[NOFILE]; // Open files
	struct inode *cwd; // Current directory
	struct cred cred; // user's credentials for the process.
	char name[16]; // Process name (debugging)
	char strace_mask_ptr[SYSCALL_AMT]; // mask for tracing syscalls
	struct mmap_info mmap_info[NMMAP];
	size_t mmap_count;
	size_t effective_largest_sz; // Largest address when including mmap
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

int
my_cpu_id(void);
void
exit(int) __attribute__((noreturn));
pid_t
fork(void);
int
growproc(int);
int
kill(int);
struct cpu *
mycpu(void);
struct proc *
myproc(void);
void
pinit(void);
void
procdump(void);
void
scheduler(void) __attribute__((noreturn));
void
sched(void);
void
setproc(struct proc *);
#ifdef __KERNEL__
void
sleep(void *, struct spinlock *);
#endif
void
userinit(void);
int
wait(int *);
void
wakeup(void *);
void
yield(void);
