// Sleeping locks

#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include <stdatomic.h>

void
initsleeplock(struct sleeplock *lk, char *name)
{
	initlock(&lk->lk, "sleep lock");
	lk->name = name;
	lk->locked = ATOMIC_FLAG_INIT;
	lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk) __acquires(lk)
{
	acquire(&lk->lk);
	while (atomic_flag_is_set(&lk->locked)) {
		sleep(lk, &lk->lk);
	}
	atomic_flag_test_and_set(&lk->locked);
	lk->pid = myproc()->pid;
	release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk) __releases(lk)
{
	acquire(&lk->lk);
	atomic_flag_clear(&lk->locked);
	lk->pid = 0;
	wakeup(lk);
	release(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
	int r;

	acquire(&lk->lk);
	r = atomic_flag_is_set(&lk->locked) && (lk->pid == myproc()->pid);
	release(&lk->lk);
	return r;
}
