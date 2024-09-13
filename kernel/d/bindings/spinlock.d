module spinlock;

extern(C)
{
	struct cpu;
	struct spinlock {
		align(1):
		uint locked; // Is the lock held?

		// For debugging:
		char *name; // Name of lock.
		cpu *cpu_; // The cpu holding the lock.
		uint[10] pcs; // The call stack (an array of program counters)
			// that locked the lock.
	}
	void
	acquire(spinlock *);
	void
	release(spinlock *);
	int
	holding(spinlock *);
}
