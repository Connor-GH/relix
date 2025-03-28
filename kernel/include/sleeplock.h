#pragma once
#include "spinlock.h"
#ifndef USE_HOST_TOOLS
#include <stdint.h>
#endif
// Long-term locks for processes
struct sleeplock {
	uint32_t locked; // Is the lock held?
	struct spinlock lk; // spinlock protecting this sleep lock

	// For debugging:
	char *name; // Name of lock.
	int pid; // Process holding lock
};
#if __KERNEL__
void
acquiresleep(struct sleeplock *s) __acquires(s);
void
releasesleep(struct sleeplock *s) __releases(s);
int
holdingsleep(struct sleeplock *);
void
initsleeplock(struct sleeplock *, char *);
#endif
