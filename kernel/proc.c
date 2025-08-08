#include "proc.h"
#include "console.h"
#include "defs.h"
#include "drivers/lapic.h"
#include "drivers/mmu.h"
#include "file.h"
#include "fs.h"
#include "kalloc.h"
#include "kernel_assert.h"
#include "kernel_signal.h"
#include "lib/compiler_attributes.h"
#include "log.h"
#include "mman.h"
#include "param.h"
#include "spinlock.h"
#include "swtch.h"
#include "syscall.h"
#include "trap.h"
#include "vm.h"
#include "x86.h"
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define W_EXITCODE(ret, signal) ((ret) << 8 | (signal))

struct {
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
	initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
my_cpu_id(void)
{
	pushcli();
	// This is a silly trick:
	// mycpu() outputs a "struct cpu *"
	// cpus is of type "struct cpu *"
	// mycpu() returns an offset of cpus,
	// i.e. (cpus + id)
	// (cpus + id) - cpus == id.
	int ret = mycpu() - cpus;
	popcli();
	return ret;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between inode_readng lapicid and running through the loop.
static int pass = 0;
struct cpu *
mycpu(void)
{
	if (readrflags() & FL_IF) {
		panic("mycpu called with interrupts enabled");
	}

	// APIC IDs are not guaranteed to be contiguous. Maybe we should have
	// a reverse map, or reserve a register to store &cpus[i].
	for (int i = 0; i < ncpu; i++) {
		int apicid = lapicid();
		if (cpus[i].apicid == apicid) {
			kernel_assert(cpus[i].apicid == i);
			return &cpus[i];
		}
	}
	// apicid is not familiar; we are probably in the early
	// stages of booting. just use the first CPU for now.
	// this is assured with "pass" because a real program would
	// use far more mycpu() than 20,000. it's just enough to boot.
	pass++;
	if (pass < 200000) {
		return &cpus[0];
	}
	panic("unknown apicid");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void)
{
	struct cpu *c;
	struct proc *p;
	pushcli();
	c = mycpu();
	p = c->proc;
	popcli();
	return p;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void)
{
	struct proc *p;

	acquire(&ptable.lock);

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == UNUSED) {
			goto found;
		}
	}

	release(&ptable.lock);
	return NULL;

found:
	p->state = EMBRYO;
	p->pid = nextpid++;

	release(&ptable.lock);

	// Allocate kernel stack.
	if ((p->kstack = kpage_alloc()) == NULL) {
		p->state = UNUSED;
		return NULL;
	}
	char *sp = p->kstack + KSTACKSIZE;

	// Leave room for trap frame.
	sp -= sizeof *p->tf;
	p->tf = (struct trapframe *)sp;

	// Set up new context to start executing at forkret,
	// which returns to trapret.
	sp -= sizeof(uintptr_t);
	*(uintptr_t *)sp = (uintptr_t)trapret;

	sp -= sizeof *p->context;
	p->context = (struct context *)sp;
	memset(p->context, 0, sizeof *p->context);
	p->context->rip = (uintptr_t)forkret;
	p->mmap_count = 0;

	// FIXME: This is an arbitrary number.
	// Right now, the virtual memory system isn't
	// good enough to just randomize this due to
	// not having leveled page tables generate as
	// needed.
	p->heap = 0x2c000000;
	p->heapsz = 0;

	p->cred.uid = 0;
	p->cred.gid = 0;
	memset(p->cred.groups, 0, sizeof(p->cred.groups));

	p->umask = S_IWGRP | S_IWOTH;

	memset(p->ptrace_mask_ptr, 0, SYSCALL_AMT);

	for (int i = 0; i < __SIG_last; i++) {
		p->sig_handlers[i] = SIG_DFL;
	}
	p->last_signal = 0;

	return p;
}

// Set up first user process.
void
userinit(void)
{
	extern char _binary_bin_initcode_start[], _binary_bin_initcode_size[];
	struct proc *p;

	p = allocproc();
	if (p == NULL) {
		panic("userinit: allocproc failed");
	}

	initproc = p;
	if ((p->pgdir = setupkvm()) == NULL) {
		panic("userinit: out of memory?");
	}
	inituvm(p->pgdir, _binary_bin_initcode_start,
	        (uintptr_t)_binary_bin_initcode_size);
	p->sz = PGSIZE;
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
	p->tf->rflags = FL_IF;
	p->tf->rsp = PGSIZE;
	p->tf->rip = 0; // beginning of initcode.S

	__safestrcpy(p->name, "initcode", sizeof(p->name));
	p->cwd = namei("/");

	// this assignment to p->state lets other cores
	// run this process. the acquire forces the above
	// writes to be visible, and the lock is also needed
	// because the assignment might not be atomic.
	acquire(&ptable.lock);

	p->state = RUNNABLE;

	release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(intptr_t n)
{
	uintptr_t sz;
	struct proc *curproc = myproc();

	sz = curproc->sz;
	if (n > 0) {
		if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0) {
			return -ENOMEM;
		}
	} else if (n < 0) {
		if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0) {
			return -EFAULT;
		}
	}
	curproc->sz = sz;
	switchuvm(curproc);
	return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
// POSIX.1-2008: fork returns a "signed integer type".
pid_t
fork(void)
{
	pid_t pid;
	struct proc *np;
	struct proc *curproc = myproc();

	// Allocate process.
	if ((np = allocproc()) == NULL) {
		return -ENOMEM;
	}

	// Copy process state from proc.
	if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == NULL) {
		kpage_free(np->kstack);
		np->kstack = NULL;
		np->state = UNUSED;
		return -EIO;
	}
	np->sz = curproc->sz;

	np->heap = curproc->heap;
	np->heapsz = curproc->heapsz;

	np->mmap_count = curproc->mmap_count;
	memcpy(np->mmap_info, curproc->mmap_info, sizeof(np->mmap_info));
	// Copying the info isn't enough. We also need to map it.
	for (int j = 0; np->mmap_info[j].length != 0; j++) {
		struct mmap_info info = np->mmap_info[j];
		if (mappages(np->pgdir, (void *)info.virt_addr, info.length, info.addr,
		             info.perm) < 0) {
			panic("Could not map page");
		}
	}
	np->parent = curproc;
	*np->tf = *curproc->tf;

	// Clear %rax so that fork returns 0 in the child.
	np->tf->rax = 0;

	for (int i = 0; i < NOFILE; i++) {
		if (curproc->ofile[i]) {
			np->ofile[i] = filedup(curproc->ofile[i]);
		}
	}
	np->cwd = inode_dup(curproc->cwd);
	np->cred = curproc->cred;
	np->umask = curproc->umask;

	__safestrcpy(np->name, curproc->name, sizeof(curproc->name));

	// when fork()ing, copy the parent's mask
	memmove(np->ptrace_mask_ptr, curproc->ptrace_mask_ptr, SYSCALL_AMT);
	pid = np->pid;

	acquire(&ptable.lock);

	np->state = RUNNABLE;

	release(&ptable.lock);

	return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
__noreturn void
exit(int status)
{
	struct proc *curproc = myproc();

	if (curproc == initproc) {
		panic("init exiting");
	}

	// Close all open files.
	for (int fd = 0; fd < NOFILE; fd++) {
		if (curproc->ofile[fd] != NULL && curproc->ofile[fd]->ref > 0) {
			(void)fileclose(curproc->ofile[fd]);
		}
	}

	begin_op();
	inode_put(curproc->cwd);
	end_op();
	curproc->cwd = NULL;
	curproc->status = status;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->parent == curproc) {
			p->parent = initproc;
			if (p->state == ZOMBIE) {
				wakeup1(initproc);
			}
		}
	}

	// Jump into the scheduler, never to return.
	curproc->state = ZOMBIE;
	sched();
	panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
// standards say that bits 15..8 are for status,
// 7..0 are for signal.
int
wait(int *wstatus)
{
	struct proc *p;
	int havekids, pid;
	struct proc *curproc = myproc();
	acquire(&ptable.lock);
	for (;;) {
		// Scan through table looking for exited children.
		havekids = 0;
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->parent != curproc) {
				continue;
			}
			havekids = 1;
			if (p->state == ZOMBIE) {
				// Found one.
				if (wstatus != NULL) {
					*wstatus = W_EXITCODE(p->status, p->last_signal);
				}
				pid = p->pid;
				kpage_free(p->kstack);
				p->kstack = NULL;
				memset(p->mmap_info, 0, sizeof(p->mmap_info));
				p->mmap_count = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = NULL;
				p->name[0] = 0;
				p->killed = 0;
				p->last_signal = 0;
				for (int i = 0; i < __SIG_last; i++) {
					p->sig_handlers[i] = SIG_DFL;
				}
				p->state = UNUSED;
				release(&ptable.lock);
				return pid;
			}
		}

		// No point waiting if we don't have any children.
		if (!havekids || curproc->killed) {
			release(&ptable.lock);
			return -ECHILD;
		}

		// Wait for children to exit.  (See wakeup1 call in proc_exit.)
		sleep(curproc, &ptable.lock); // DOC: wait-sleep
	}
}

struct proc *
last_proc_ran(void)
{
	struct proc *p;
	acquire(&ptable.lock);
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p != initproc && p->pid != 2 &&
		    (p->state == RUNNING || p->state == SLEEPING)) {
			release(&ptable.lock);
			return p;
		}
	}
	release(&ptable.lock);
	return NULL;
}
#define GET(reg)                                            \
	({                                                        \
		uintptr_t reg;                                          \
		__asm__ __volatile__("mov %%" #reg ", %0" : "=r"(reg)); \
		reg;                                                    \
	})
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
	int ran = 0;
	struct cpu *c = mycpu();
	c->proc = 0;

	for (;;) {
		// Enable interrupts on this processor.
		sti();

		// Loop over process table looking for process to run.
		acquire(&ptable.lock);
		for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->state != RUNNABLE) {
				continue;
			}

			// Switch to chosen process.  It is the process's job
			// to release ptable.lock and then reacquire it
			// before jumping back to us.
			ran = 1;
			c->proc = p;
			switchuvm(p);
			p->state = RUNNING;

			// Depending on the bits set, this may also set the extended state.
			__asm__ __volatile__("fxsave %0" : "=m"(c->proc->legacy_fpu_state));
			swtch(&(c->scheduler), p->context);
			__asm__ __volatile__("fxrstor %0" : : "m"(c->proc->legacy_fpu_state));
			switchkvm();

			// Process is done running for now.
			// It should have changed its p->state before coming back.
			c->proc = NULL;
		}
		release(&ptable.lock);
		if (ran == 0) {
			hlt();
		}
	}
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
	int intena;
	struct proc *p = myproc();

	if (!holding(&ptable.lock)) {
		panic("sched ptable.lock");
	}
	if (mycpu()->ncli != 1) {
		panic("sched locks");
	}
	if (p->state == RUNNING) {
		panic("sched running");
	}
	if (readrflags() & FL_IF) {
		panic("sched interruptible");
	}
	intena = mycpu()->intena;
	swtch(&p->context, mycpu()->scheduler);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	acquire(&ptable.lock); // DOC: yieldlock
	myproc()->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

static int first = 1;

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
	// Still holding ptable.lock from scheduler.
	release(&ptable.lock);

	if (first) {
		// Some initialization functions must be run in the context
		// of a regular process (e.g., they call sleep), and thus cannot
		// be run from main().
		first = 0;
		inode_init(ROOTDEV);
		initlog(ROOTDEV);
	}

	// Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
	struct proc *p = myproc();

	if (p == NULL) {
		panic("sleep");
	}

	if (lk == NULL) {
		panic("sleep without lk");
	}

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if (lk != &ptable.lock) { // DOC: sleeplock0
		acquire(&ptable.lock); // DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	p->chan = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = NULL;

	// Reacquire original lock.
	if (lk != &ptable.lock) { // DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == SLEEPING && p->chan == chan) {
			p->state = RUNNABLE;
		}
	}
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

// When we enter the signal handler, we SHOULDN'T have to
// pop anything off of the stack. %rdi will contain the one
// argument, and so we put the signal in there. %rip should
// point to the signal handler so that we can go there. The
// stack should contain the return address (%rip).
static void
copy_signal_to_stack(struct proc *proc, int signal)
{
	uintptr_t sp = proc->tf->rsp;
	uintptr_t ustack = proc->tf->rip;

	sp -= sizeof(uintptr_t);
	if (copyout(proc->pgdir, sp, &ustack, sizeof(ustack)) < 0) {
		panic("failed to copyout");
	}
	proc->tf->rdi = signal;
	proc->tf->rip = (uintptr_t)proc->sig_handlers[signal];
	proc->tf->rsp = sp;
}

// Send the process with the given pid the given signal.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(pid_t pid, int signal)
{
	acquire(&ptable.lock);
	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid == pid) {
			p->last_signal = signal;

			if (p->sig_handlers[signal] == SIG_DFL) {
				switch (signal) {
				case SIGABRT:
				case SIGALRM:
				case SIGHUP:
				case SIGQUIT:
				case SIGSYS:
				case SIGTERM:
				case SIGTRAP:
				case SIGUSR1:
				case SIGUSR2:
				case SIGVTALRM:
				case SIGXCPU:
				case SIGXFSZ:
				case SIGSEGV:
				case SIGBUS:
				case SIGILL:
				case SIGKILL:
				case SIGPIPE:
				case SIGINT:
					p->killed = 1;
					break;
				case SIGTSTP:
				case SIGTTIN:
				case SIGTTOU:
				case SIGSTOP:
					p->state = STOPPED;
					break;
				case SIGCONT:
					// Continue
					p->state = RUNNABLE;
					break;
				default:
				// Ignore signals
				case SIGURG:
				case SIGWINCH:
					break;
				}
			} else {
				copy_signal_to_stack(p, signal);
			}
			// Wake process from sleep if necessary.
			if (p->state == SLEEPING) {
				p->state = RUNNABLE;
			}
			release(&ptable.lock);
			return 0;
		}
	}
	release(&ptable.lock);
	return -1;
}

sighandler_t
kernel_attach_signal(int signum, sighandler_t handler)
{
	if (signum == SIGFPE || signum == SIGSEGV || signum == SIGBUS ||
	    signum == SIGILL) {
		return SIG_ERR;
	}
	if (signum >= __SIG_last || signum < 0) {
		return SIG_ERR;
	} else {
		sighandler_t old = myproc()->sig_handlers[signum];
		myproc()->sig_handlers[signum] = handler;
		return old;
	}
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
	static char *states[] = {
		[UNUSED] = "unused",     [EMBRYO] = "embryo",   [SLEEPING] = "sleep",
		[RUNNABLE] = "runnable", [RUNNING] = "running", [ZOMBIE] = "zombie",
		[STOPPED] = "stopped",
	};
	char *state;
	uintptr_t pc[10];

	for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == UNUSED) {
			continue;
		}
		if (p->state >= 0 && p->state < NELEM(states) && states[p->state]) {
			state = states[p->state];
		} else {
			state = "???";
		}
		vga_cprintf("%d %s %s", p->pid, state, p->name);
		if (p->state == SLEEPING) {
			getcallerpcs(pc);
			for (int i = 0; i < 10 && pc[i] != 0; i++) {
				vga_cprintf(" %lx", pc[i]);
			}
		}
		vga_cprintf("\n");
	}
}

void
sleep_on_ms(time_t ms)
{
	time_t ticks0;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < ms) {
		sleep(&ticks, &tickslock);
	}
	release(&tickslock);
}

bool
is_in_group(gid_t group, struct cred *cred)
{
	for (int i = 0; i < MAXGROUPS; i++) {
		if (group == cred->groups[i]) {
			return true;
		}
	}
	return false;
}
