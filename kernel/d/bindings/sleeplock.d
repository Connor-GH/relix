module sleeplock;
extern(C) @nogc __gshared:
import spinlock:spinlock;

struct sleeplock {
	align(1):
	uint locked; // Is the lock held?
	spinlock lk; // spinlock protecting this sleep lock

	// For debugging:
	char *name; // Name of lock.
	int pid; // Process holding lock
}
void
acquiresleep(sleeplock *);
void
releasesleep(sleeplock *);
int
holdingsleep(sleeplock *);
