// Sleeping locks

#include "sleeplock.h"
#include "proc.h"
#include "spinlock.h"
#include <stdatomic.h>

void
initsleeplock(struct sleeplock *lk, const char *name)
{
	initlock(&lk->lk, "sleep lock");
	lk->name = name;
	lk->locked = 0;
	lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk) __acquires(lk)
{
	acquire(&lk->lk);
	while (atomic_load(&lk->locked)) {
		sleep(lk, &lk->lk);
	}
	atomic_exchange_explicit(&lk->locked, 1, memory_order_acquire);
	lk->pid = myproc()->pid;
	release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk) __releases(lk)
{
	acquire(&lk->lk);
	atomic_store_explicit(&lk->locked, 0, memory_order_release);
	lk->pid = 0;
	wakeup(lk);
	release(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
	int r;

	acquire(&lk->lk);
	r = atomic_load(&lk->locked) && (lk->pid == myproc()->pid);
	release(&lk->lk);
	return r;
}
