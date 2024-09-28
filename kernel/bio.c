// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "console.h"
#include "x86.h"
#include "defs.h"
#include "trap.h"
#include "fs.h"
#include "buf.h"
#include "ide.h"
#include "mp.h"
#include "macros.h"

extern int ncpu;
#define NBUCKET NCPU

struct {
	struct buf buf[NBUF];

	// Linked list of all buffers, through prev/next.
	// Sorted by how recently the buffer was used.
	// head.next is most recent, head.prev is least.
	// Hash bucket
	struct buf bucket[NBUCKET];
	struct spinlock bucket_lock[NBUCKET];
} bcache;

int
hash(uint blockno)
{
	return blockno % min(NBUCKET, ncpu);
}

void
binit(void)
{
	struct buf *b;

	for (int i = 0; i < min(NBUCKET, ncpu); i++) {
		initlock(&bcache.bucket_lock[i], "bcache.bucket");
		bcache.bucket[i].next = &bcache.bucket[i];
		bcache.bucket[i].prev = &bcache.bucket[i];
	}

	// Create hash table of buffers
	for (b = bcache.buf; b < bcache.buf + NBUF; b++) {
		int hi = hash(b->blockno);
		b->next = bcache.bucket[hi].next;
		b->prev = &bcache.bucket[hi];
		initsleeplock(&b->lock, "buffer");
		bcache.bucket[hi].next->prev = b;
		bcache.bucket[hi].next = b;
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static int i = 0;

static struct buf *
bget(uint dev, uint blockno)
{
	struct buf *b;
	int hi = hash(blockno);

	// Find cache from bucket hi.
	acquire(&bcache.bucket_lock[hi]);
	for (b = bcache.bucket[hi].next; b != &bcache.bucket[hi]; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			release(&bcache.bucket_lock[hi]);
			acquiresleep(&b->lock);
			return b;
		}
	}
	// Not cached; recycle an unused buffer.
	// Even if refcnt==0, B_DIRTY indicates a buffer is in use
	// because log.c has modified it but not yet committed it.
	for (b = bcache.bucket[hi].prev; b != &bcache.bucket[hi]; b = b->prev) {
		if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
			b->dev = dev;
			b->blockno = blockno;
			b->flags = 0;
			b->refcnt = 1;
			release(&bcache.bucket_lock[hi]);
			acquiresleep(&b->lock);
			return b;
		}
	}
	release(&bcache.bucket_lock[hi]);
	// Find Recycle the least recently used (LRU) unused buffer.
	if (i >= min(NBUCKET, ncpu))
		i = 0;
	for (; i < min(NBUCKET, ncpu); i++) {
		// optimization: skip already checked hashed blockno
		if (i == hi)
			continue;
		acquire(&bcache.bucket_lock[i]);
		for (b = bcache.bucket[i].next; b != &bcache.bucket[i]; b = b->next) {
			if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
				//b->prev->next = b->next;
				//b->next->prev = b->prev;
				b->dev = dev;
				b->blockno = blockno;
				b->flags = 0;
				b->refcnt = 1;
				release(&bcache.bucket_lock[i]);
				acquiresleep(&b->lock);
				return b;
			}
		}
		release(&bcache.bucket_lock[i]);
	}
	panic("bget: no buffers2");
}

// Return a locked buf with the contents of the indicated block.
struct buf *
bread(uint dev, uint blockno)
{
	struct buf *b;
	b = bget(dev, blockno);
	if ((b->flags & B_VALID) == 0) {
		iderw(b);
	}
	return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
	if (unlikely(!holdingsleep(&b->lock)))
		panic("bwrite");
	b->flags |= B_DIRTY;
	iderw(b);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
	if (unlikely(!holdingsleep(&b->lock)))
		panic("brelse");

	releasesleep(&b->lock);

	int hi = hash(b->blockno);
	acquire(&bcache.bucket_lock[hi]);
	b->refcnt--;
	if (b->refcnt == 0) {
		b->next->prev = b->prev;
		b->prev->next = b->next;
		b->next = bcache.bucket[hi].next;
		b->prev = &bcache.bucket[hi];
		bcache.bucket[hi].next->prev = b;
		bcache.bucket[hi].next = b;
	}
	release(&bcache.bucket_lock[hi]);
}
