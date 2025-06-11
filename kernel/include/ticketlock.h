#pragma once
#include <stdint.h>

struct ticketlock {
	_Atomic(uint64_t) now_serving;
	_Atomic(uint64_t) next_ticket;
	// uint64_t next_ticket;

	// For debugging:
	const char *name; // Name of lock.
	struct cpu *cpu; // The cpu holding the lock.
	uintptr_t pcs[10]; // The call stack (an array of program counters)
	// that locked the lock.
};

int holding_ticketlock(struct ticketlock *tl);

void init_ticketlock(struct ticketlock *tl, const char *name);

void acquire_ticketlock(struct ticketlock *tl);

void release_ticketlock(struct ticketlock *tl);
