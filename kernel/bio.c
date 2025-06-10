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
// * When done with the buffer, call block_release.
// * Do not use the buffer after calling block_release.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include <stddef.h>
#include <stdint.h>
#include "kernel_assert.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "console.h"
#include "buf.h"
#include "ide.h"
#include "macros.h"

extern int ncpu;
#define NBUCKET NCPU

struct {
	struct block_buffer buf[NBUF];

	// Linked list of all buffers, through prev/next.
	// Sorted by how recently the buffer was used.
	// head.next is most recent, head.prev is least.
	// Hash bucket
	struct block_buffer bucket[NBUCKET];
	struct spinlock bucket_lock[NBUCKET];
} block_cache;

static uint64_t
hash(uint64_t blockno)
{
	return blockno % min(NBUCKET, ncpu);
}

void
block_init(void)
{
	struct block_buffer *b;

	for (size_t i = 0; i < min(NBUCKET, ncpu); i++) {
		initlock(&block_cache.bucket_lock[i], "block_cache.bucket");
		block_cache.bucket[i].next = &block_cache.bucket[i];
		block_cache.bucket[i].prev = &block_cache.bucket[i];
	}

	// Create hash table of buffers
	for (b = block_cache.buf; b < block_cache.buf + NBUF; b++) {
		uint64_t hi = hash(b->blockno);
		b->next = block_cache.bucket[hi].next;
		b->prev = &block_cache.bucket[hi];
		initsleeplock(&b->lock, "buffer");
		block_cache.bucket[hi].next->prev = b;
		block_cache.bucket[hi].next = b;
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct block_buffer *
block_get(dev_t dev, uint64_t blockno) __acquires(&b->lock)
{
	struct block_buffer *b;
	size_t hi = hash(blockno);

	// Find cache from bucket hi.
	acquire(&block_cache.bucket_lock[hi]);
	for (b = block_cache.bucket[hi].next; b != &block_cache.bucket[hi]; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			release(&block_cache.bucket_lock[hi]);
			acquiresleep(&b->lock);
			return b;
		}
	}
	// Not cached; recycle an unused buffer.
	// Even if refcnt==0, B_DIRTY indicates a buffer is in use
	// because log.c has modified it but not yet committed it.
	for (b = block_cache.bucket[hi].prev; b != &block_cache.bucket[hi]; b = b->prev) {
		if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
			b->dev = dev;
			b->blockno = blockno;
			b->flags = 0;
			b->refcnt = 1;
			release(&block_cache.bucket_lock[hi]);
			acquiresleep(&b->lock);
			return b;
		}
	}
	release(&block_cache.bucket_lock[hi]);
	// Find Recycle the least recently used (LRU) unused buffer.
	for (size_t j = 0; j < min(NBUCKET, ncpu); j++) {
		size_t i = (hi + j) % min(NBUCKET, ncpu);
		acquire(&block_cache.bucket_lock[i]);
		for (b = block_cache.bucket[i].next; b != &block_cache.bucket[i]; b = b->next) {
			if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
				//b->prev->next = b->next;
				//b->next->prev = b->prev;
				b->dev = dev;
				b->blockno = blockno;
				b->flags = 0;
				b->refcnt = 1;
				release(&block_cache.bucket_lock[i]);
				acquiresleep(&b->lock);
				return b;
			}
		}
		release(&block_cache.bucket_lock[i]);
	}
	panic("bget: no buffers2");
}

// Return a locked buf with the contents of the indicated block.
struct block_buffer *
block_read(dev_t dev, uint64_t blockno) __acquires(&b->lock)
{
	struct block_buffer *b;
	b = block_get(dev, blockno);
	if ((b->flags & B_VALID) == 0) {
		iderw(b);
	}
	return b;
}

// Write b's contents to disk.  Must be locked.
void
block_write(struct block_buffer *b) __must_hold(&b->lock)
{
	kernel_assert(holdingsleep(&b->lock));
	b->flags |= B_DIRTY;
	iderw(b);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
block_release(struct block_buffer *b) __releases(&b->lock)
{
	kernel_assert(holdingsleep(&b->lock));

	releasesleep(&b->lock);

	size_t hi = hash(b->blockno);
	acquire(&block_cache.bucket_lock[hi]);
	b->refcnt--;
	if (b->refcnt == 0) {
		b->next->prev = b->prev;
		b->prev->next = b->next;
		b->next = block_cache.bucket[hi].next;
		b->prev = &block_cache.bucket[hi];
		block_cache.bucket[hi].next->prev = b;
		block_cache.bucket[hi].next = b;
	}
	release(&block_cache.bucket_lock[hi]);
}
