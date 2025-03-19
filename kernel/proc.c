#include "mman.h"
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include "drivers/mmu.h"
#include "drivers/lapic.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "kalloc.h"
#include "console.h"
#include "vm.h"
#include "log.h"
#include "swtch.h"
#include "syscall.h"
#include "file.h"
#include "fs.h"
#include "trap.h"
#include "compiler_attributes.h"
#include "types.h"
#include "kernel_signal.h"

#define W_EXITCODE(ret, signal) ((ret) << 8 | (signal))

struct {
	struct spinlock lock;
	struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void
forkret(void);
extern void
trapret(void);

static void
wakeup1(void *chan);

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
	int apicid, i;

	if (readeflags() & FL_IF)
		panic("mycpu called with interrupts enabled\n");

	apicid = lapicid();
	// APIC IDs are not guaranteed to be contiguous. Maybe we should have
	// a reverse map, or reserve a register to store &cpus[i].
	for (i = 0; i < ncpu; ++i) {
		if (cpus[i].apicid == apicid)
			return &cpus[i];
	}
	// apicid is not familiar; we are probably in the early
	// stages of booting. just use the first CPU for now.
	// this is assured with "pass" because a real program would
	// use far more mycpu() than 20,000. it's just enough to boot.
	pass++;
	if (pass < 200000)
		return &cpus[0];
	panic("unknown apicid\n");
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
	char *sp;

	acquire(&ptable.lock);

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == UNUSED)
			goto found;

	release(&ptable.lock);
	return 0;

found:
	p->state = EMBRYO;
	p->pid = nextpid++;

	release(&ptable.lock);

	// Allocate kernel stack.
	if ((p->kstack = kpage_alloc()) == 0) {
		p->state = UNUSED;
		return 0;
	}
	sp = p->kstack + KSTACKSIZE;

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
	p->context->eip = (uintptr_t)forkret;

	p->cred.uid = 0;
	p->cred.gid = 0;

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
	if (p == NULL)
		panic("userinit: allocproc failed");

	initproc = p;
	if ((p->pgdir = setupkvm()) == 0)
		panic("userinit: out of memory?");
	inituvm(p->pgdir, _binary_bin_initcode_start,
					(uintptr_t)_binary_bin_initcode_size);
	p->sz = PGSIZE;
	memset(p->tf, 0, sizeof(*p->tf));
	p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
	p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
	p->tf->eflags = FL_IF;
	p->tf->esp = PGSIZE;
	p->tf->eip = 0; // beginning of initcode.S

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
growproc(int n)
{
	uintptr_t sz;
	struct proc *curproc = myproc();

	sz = curproc->sz;
	if (n > 0) {
		if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
			return -ENOMEM;
	} else if (n < 0) {
		if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
			return -EFAULT;
	}
	curproc->sz = sz;
	if (sz > curproc->effective_largest_sz)
		curproc->effective_largest_sz = curproc->sz;
	switchuvm(curproc);
	return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
// POSIX.1-2008: fork is a "signed integer type".
pid_t
fork(bool virtual)
{
	int i;
	pid_t pid;
	struct proc *np;
	struct proc *curproc = myproc();

	// Allocate process.
	if ((np = allocproc()) == 0) {
		return -ENOMEM;
	}

	// Copy process state from proc.
	if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
		kpage_free(np->kstack);
		np->kstack = 0;
		np->state = UNUSED;
		return -EIO;
	}
	np->sz = curproc->sz;
	np->effective_largest_sz = curproc->effective_largest_sz;
	// Only do this for regular fork().
	if (!virtual) {
		np->mmap_count = curproc->mmap_count;
		memcpy(np->mmap_info, curproc->mmap_info, sizeof(np->mmap_info));
		// Copying the info isn't enough. We also need to map it.
		for (int j = 0; np->mmap_info[j].file != NULL; j++) {
			struct mmap_info info = np->mmap_info[j];
			if (mappages(np->pgdir, (void *)info.virt_addr, info.length, info.addr, info.perm) < 0) {
				panic("Could not map page");
			}
		}
	}
	np->parent = curproc;
	*np->tf = *curproc->tf;

	// Clear %eax so that fork returns 0 in the child.
	np->tf->eax = 0;

	for (i = 0; i < NOFILE; i++)
		if (curproc->ofile[i])
			np->ofile[i] = filedup(curproc->ofile[i]);
	np->cwd = inode_dup(curproc->cwd);

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
	struct proc *p;
	int fd;

	if (curproc == initproc)
		panic("init exiting");

	// Close all open files.
	for (fd = 0; fd < NOFILE; fd++) {
		if (curproc->ofile[fd]) {
			fileclose(curproc->ofile[fd]);
			curproc->ofile[fd] = 0;
		}
	}

	begin_op();
	inode_put(curproc->cwd);
	end_op();
	curproc->cwd = 0;
	curproc->status = status;

	acquire(&ptable.lock);

	// Parent might be sleeping in wait().
	wakeup1(curproc->parent);

	// Pass abandoned children to init.
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->parent == curproc) {
			p->parent = initproc;
			if (p->state == ZOMBIE)
				wakeup1(initproc);
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
			if (p->parent != curproc)
				continue;
			havekids = 1;
			if (p->state == ZOMBIE) {
				// Found one.
				if (wstatus != NULL)
					*wstatus = W_EXITCODE(p->status, p->last_signal);
				pid = p->pid;
				kpage_free(p->kstack);
				p->kstack = 0;
				memset(p->mmap_info, 0, sizeof(p->mmap_info));
				p->mmap_count = 0;
				freevm(p->pgdir);
				p->pid = 0;
				p->parent = 0;
				p->name[0] = 0;
				p->killed = 0;
				p->last_signal = 0;
				for (int i = 0; i < __SIG_last; i++)
					p->sig_handlers[i] = SIG_DFL;
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
		sleep(curproc, &ptable.lock); //DOC: wait-sleep
	}
}

struct proc *
last_proc_ran(void)
{

	struct proc *p;
	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if (p != initproc && p->pid != 2 && (p->state == RUNNING || p->state == SLEEPING)) {
			release(&ptable.lock);
			return p;
    }
  }
	release(&ptable.lock);
	return NULL;
}
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
	struct proc *p;
	int ran = 0;
	struct cpu *c = mycpu();
	c->proc = 0;

	for (;;) {
		// Enable interrupts on this processor.
		sti();

		// Loop over process table looking for process to run.
		acquire(&ptable.lock);
		for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
			if (p->state != RUNNABLE)
				continue;

			// Switch to chosen process.  It is the process's job
			// to release ptable.lock and then reacquire it
			// before jumping back to us.
			ran = 1;
			c->proc = p;
			switchuvm(p);
			p->state = RUNNING;

			__asm__ __volatile__("fxsave %0" : "=m" (c->proc->fpu_state));
			swtch(&(c->scheduler), p->context);
			__asm__ __volatile__("fxrstor %0" : : "m" (c->proc->fpu_state));
			switchkvm();

			// Process is done running for now.
			// It should have changed its p->state before coming back.
			c->proc = 0;
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

	if (!holding(&ptable.lock))
		panic("sched ptable.lock");
	if (mycpu()->ncli != 1)
		panic("sched locks");
	if (p->state == RUNNING)
		panic("sched running");
	if (readeflags() & FL_IF)
		panic("sched interruptible");
	intena = mycpu()->intena;
	swtch(&p->context, mycpu()->scheduler);
	mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
	acquire(&ptable.lock); //DOC: yieldlock
	myproc()->state = RUNNABLE;
	sched();
	release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
	static int first = 1;
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

	if (p == 0)
		panic("sleep");

	if (lk == 0)
		panic("sleep without lk");

	// Must acquire ptable.lock in order to
	// change p->state and then call sched.
	// Once we hold ptable.lock, we can be
	// guaranteed that we won't miss any wakeup
	// (wakeup runs with ptable.lock locked),
	// so it's okay to release lk.
	if (lk != &ptable.lock) { //DOC: sleeplock0
		acquire(&ptable.lock); //DOC: sleeplock1
		release(lk);
	}
	// Go to sleep.
	p->chan = chan;
	p->state = SLEEPING;

	sched();

	// Tidy up.
	p->chan = 0;

	// Reacquire original lock.
	if (lk != &ptable.lock) { //DOC: sleeplock2
		release(&ptable.lock);
		acquire(lk);
	}
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
	struct proc *p;

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
		if (p->state == SLEEPING && p->chan == chan)
			p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
	acquire(&ptable.lock);
	wakeup1(chan);
	release(&ptable.lock);
}

static void
copy_signal_to_stack(struct proc *proc, int signal)
{
	uintptr_t sp = proc->tf->esp;
	uintptr_t ustack[2];
	ustack[0] = proc->tf->eip;
	ustack[1] = (uintptr_t)proc->sig_handlers[signal];
	sp -= sizeof(sighandler_t (*)(int));
	if (copyout(proc->pgdir, sp, ustack, sizeof(ustack)) < 0) {
		panic("failed to copyout");
	}
	proc->tf->rdi = signal;
	proc->tf->eip = ustack[1];
	proc->tf->esp = sp;
}

// Send the process with the given pid the given signal.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(pid_t pid, int signal)
{
	struct proc *p;

	acquire(&ptable.lock);
	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->pid == pid) {
			p->last_signal = signal;

			if (signal == SIGFPE || signal == SIGSEGV || signal == SIGBUS ||
				signal == SIGILL || signal == SIGKILL || p->sig_handlers[signal] == SIG_DFL) {
				p->killed = 1;
			} else {
				copy_signal_to_stack(p, signal);
			}
			// Wake process from sleep if necessary.
			if (p->state == SLEEPING)
				p->state = RUNNABLE;
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
		signum == SIGILL)
		return SIG_ERR;
	if (signum >= __SIG_last || signum < 0) {
		return SIG_ERR;
	} else {
		myproc()->sig_handlers[signum] = handler;
		return 0;
	}
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
	static char *states[] = {
		[UNUSED] = "unused",	 [EMBRYO] = "embryo",	 [SLEEPING] = "sleep ",
		[RUNNABLE] = "runble", [RUNNING] = "run   ", [ZOMBIE] = "zombie"
	};
	int i;
	struct proc *p;
	char *state;
	uintptr_t pc[10];

	for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if (p->state == UNUSED)
			continue;
		if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
			state = states[p->state];
		else
			state = "???";
		vga_cprintf("%d %s %s", p->pid, state, p->name);
		if (p->state == SLEEPING) {
			getcallerpcs((uintptr_t *)p->context->ebp, pc);
			for (i = 0; i < 10 && pc[i] != 0; i++)
				vga_cprintf(" %lx", pc[i]);
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
