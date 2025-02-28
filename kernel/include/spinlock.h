#pragma once
#ifndef USE_HOST_TOOLS
#include <stdint.h>
#else
#undef __always_inline
#undef __nonnull
#endif

#include "compiler_attributes.h"
// Mutual exclusion lock.
struct spinlock {
	uint32_t locked; // Is the lock held?

	// For debugging:
	char *name; // Name of lock.
	struct cpu *cpu; // The cpu holding the lock.
	uintptr_t pcs[10]; // The call stack (an array of program counters)
		// that locked the lock.
};
void
acquire(struct spinlock *s) __acquires(s);
void
getcallerpcs(void *, uintptr_t *);
int
holding(struct spinlock *);
void
initlock(struct spinlock *, char *);
void
release(struct spinlock *s) __releases(s);
void
pushcli(void);
void
popcli(void);
