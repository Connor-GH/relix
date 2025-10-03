// Mutual exclusion spin locks.

#include "spinlock.h"
#include "console.h"
#include "kernel_assert.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "symbols.h"
#include "x86.h"

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

void
initlock(struct spinlock *lk, const char *name)
{
	lk->name = name;
	lk->locked = 0;
	lk->cpu = NULL;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquire(struct spinlock *lk) __acquires(lk)
{
	pushcli(); // disable interrupts to avoid deadlock.
	kernel_assert(!holding(lk));

	while (atomic_exchange_explicit(&lk->locked, 1, memory_order_acquire) != 0)
		;
	__acquire(lk);
	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that the critical section's memory
	// references happen after the lock is acquired.
	atomic_thread_fence(memory_order_seq_cst);
	// Record info about lock acquisition for debugging.
	lk->cpu = mycpu();
	getcallerpcs(lk->pcs);
}

// Release the lock.
void
release(struct spinlock *lk) __releases(lk)
{
	kernel_assert_unlocked(holding(lk));

	lk->pcs[0] = 0;
	lk->cpu = NULL;

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that all the stores in the critical
	// section are visible to other cores before the lock is released.
	// Both the C compiler and the hardware may re-order loads and
	// stores; atomic_thread_fence() tells them both not to.
	atomic_thread_fence(memory_order_seq_cst);

	// Release the lock, equivalent to lk->locked = 0.
	// This code can't use a C assignment, since it might
	// not be atomic.
	atomic_store_explicit(&lk->locked, 0, memory_order_release);
	__release(lk);

	popcli();
}

// Record the current call stack in pcs[] by following the %rbp chain.
void
getcallerpcs(uintptr_t pcs[])
{
	uintptr_t *rbp;
#if __x86_64__
	asm volatile("mov %%rbp, %0" : "=r"(rbp));
#endif
	getcallerpcs_with_bp(pcs, rbp, 10);
}
void
getcallerpcs_with_bp(uintptr_t pcs[], uintptr_t *rbp, size_t size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (rbp == NULL || rbp < (uintptr_t *)KERNBASE ||
		    rbp == (uintptr_t *)0xffffffff) {
			break;
		}
		pcs[i] = rbp[1]; // saved %rip
		rbp = (uintptr_t *)rbp[0]; // saved %rbp
	}
	for (; i < size; i++) {
		pcs[i] = 0;
	}
}

// Check whether this cpu is holding the lock.
int
holding(struct spinlock *lock)
{
	int r;
	pushcli();
	r = atomic_load(&lock->locked) && lock->cpu == mycpu();
	popcli();
	return r;
}

// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void
pushcli(void)
{
	int rflags = readrflags();

	cli();
	if (mycpu()->ncli == 0) {
		mycpu()->intena = rflags & FL_IF;
	}
	mycpu()->ncli += 1;
}

void
popcli(void)
{
	if (readrflags() & FL_IF) {
		panic("popcli - interruptible");
	}
	if (--mycpu()->ncli < 0) {
		panic("popcli");
	}
	if (mycpu()->ncli == 0 && mycpu()->intena) {
		sti();
	}
}
