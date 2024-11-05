#pragma once
#ifndef USE_HOST_TOOLS
#include <stdint.h>
#endif
// Mutual exclusion lock.
struct spinlock {
	uint32_t locked; // Is the lock held?

	// For debugging:
	char *name; // Name of lock.
	struct cpu *cpu; // The cpu holding the lock.
	uint32_t pcs[10]; // The call stack (an array of program counters)
		// that locked the lock.
};
void
acquire(struct spinlock *);
void
getcallerpcs(void *, uint32_t *);
int
holding(struct spinlock *);
void
initlock(struct spinlock *, char *);
void
release(struct spinlock *);
void
pushcli(void);
void
popcli(void);
