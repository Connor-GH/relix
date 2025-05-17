#pragma once
#ifndef _SPINLOCK_H
#define _SPINLOCK_H
#ifndef USE_HOST_TOOLS
#include <stdint.h>
#include <stddef.h>
#else
// When making mkfs, the libc
// implementation defines these.
// Undefine them so we can use our own.
#undef __always_inline
#undef __nonnull
#endif
#include <stdatomic.h>
#include "lib/compiler_attributes.h"
// Mutual exclusion lock.
struct spinlock {
	atomic_flag locked;

	// For debugging:
	char *name; // Name of lock.
	struct cpu *cpu; // The cpu holding the lock.
	uintptr_t pcs[10]; // The call stack (an array of program counters)
		// that locked the lock.
};
#if __KERNEL__
void
acquire(struct spinlock *s) __acquires(s);
void
getcallerpcs(void *, uintptr_t *);
void
getcallerpcs_with_bp(uintptr_t pcs[], uintptr_t *rbp, size_t size);
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
#endif
#endif /* _SPINLOCK_H */
