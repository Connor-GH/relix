// Mutual exclusion spin locks.

#include <stdint.h>
#include "drivers/memlayout.h"
#include "drivers/mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "console.h"
#include "kernel_assert.h"

void
initlock(struct spinlock *lk, char *name)
{
	lk->name = name;
	lk->locked = 0;
	lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquire(struct spinlock *lk) __acquires(lk)
{
	pushcli(); // disable interrupts to avoid deadlock.
	if (holding(lk))
		uart_printf("%s\n", lk->name);
	kernel_assert(!holding(lk));

	while (__sync_lock_test_and_set(&lk->locked, 1) != 0)
		;
	__acquire(lk);
	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that the critical section's memory
	// references happen after the lock is acquired.
	__sync_synchronize();
	// Record info about lock acquisition for debugging.
	lk->cpu = mycpu();
	getcallerpcs(&lk, lk->pcs);
}

// Release the lock.
void
release(struct spinlock *lk) __releases(lk)
{
	kernel_assert(holding(lk));

	lk->pcs[0] = 0;
	lk->cpu = 0;

	// Tell the C compiler and the processor to not move loads or stores
	// past this point, to ensure that all the stores in the critical
	// section are visible to other cores before the lock is released.
	// Both the C compiler and the hardware may re-order loads and
	// stores; __sync_synchronize() tells them both not to.
	__sync_synchronize();

	// Release the lock, equivalent to lk->locked = 0.
	// This code can't use a C assignment, since it might
	// not be atomic. A real OS would use C atomics here.
	__sync_lock_release(&lk->locked);
	__release(lk);

	popcli();
}

// Record the current call stack in pcs[] by following the %rbp chain.
void
getcallerpcs(void *v, uintptr_t pcs[])
{
	uintptr_t *rbp;
#if X86_64
	asm volatile("mov %%rbp, %0" : "=r"(rbp));
#else
	rbp = (uintptr_t *)v - 2;
#endif
	getcallerpcs_with_bp(pcs, rbp, 10);

}
void
getcallerpcs_with_bp(uintptr_t pcs[], uintptr_t *rbp, size_t size)
{
	int i;

	for (i = 0; i < size; i++) {
		if (rbp == 0 || rbp < (uintptr_t *)KERNBASE ||
				rbp == (uintptr_t *)0xffffffff)
			break;
		pcs[i] = rbp[1]; // saved %rip
		rbp = (uintptr_t *)rbp[0]; // saved %rbp
	}
	for (; i < size; i++)
		pcs[i] = 0;
}

// Check whether this cpu is holding the lock.
int
holding(struct spinlock *lock)
{
	int r;
	pushcli();
	r = lock->locked && lock->cpu == mycpu();
	popcli();
	return r;
}

// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void
pushcli(void)
{
	int rflags;

	rflags = readrflags();
	cli();
	if (mycpu()->ncli == 0)
		mycpu()->intena = rflags & FL_IF;
	mycpu()->ncli += 1;
}

void
popcli(void)
{
	if (readrflags() & FL_IF)
		panic("popcli - interruptible");
	if (--mycpu()->ncli < 0)
		panic("popcli");
	if (mycpu()->ncli == 0 && mycpu()->intena)
		sti();
}
