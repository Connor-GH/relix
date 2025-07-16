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

#include "buf.h"
#include "console.h"
#include "ide.h"
#include "kernel_assert.h"
#include "param.h"
#include "sleeplock.h"
#include "spinlock.h"
#include <stdint.h>

struct {
	struct spinlock lock;
	struct block_buffer buf[NBUF];

	// Linked list of all buffers, through prev/next.
	// Sorted by how recently the buffer was used.
	// head.next is most recent, head.prev is least.
	struct block_buffer head;
} block_cache;

void
block_init(void)
{
	initlock(&block_cache.lock, "block_cache");

	// Create hash table of buffers
	block_cache.head.next = &block_cache.head;
	block_cache.head.prev = &block_cache.head;

	for (struct block_buffer *b = block_cache.buf; b < block_cache.buf + NBUF;
	     b++) {
		b->next = block_cache.head.next;
		b->prev = &block_cache.head;

		initsleeplock(&b->lock, "buffer");
		block_cache.head.next->prev = b;
		block_cache.head.next = b;
	}
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct block_buffer *
block_get(dev_t dev, uint64_t blockno) __acquires(&b->lock)
{
	struct block_buffer *b;

	acquire(&block_cache.lock);

	// Is the block already cached?
	for (b = block_cache.head.next; b != &block_cache.head; b = b->next) {
		if (b->dev == dev && b->blockno == blockno) {
			b->refcnt++;
			release(&block_cache.lock);
			acquiresleep(&b->lock);
			return b;
		}
	}
	// Not cached; recycle an unused buffer.
	// Even if refcnt == 0, B_DIRTY indicates a buffer is in use
	// because log.c has modified it but not yet committed it.
	for (b = block_cache.head.prev; b != &block_cache.head; b = b->prev) {
		if (b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
			b->dev = dev;
			b->blockno = blockno;
			b->flags = 0;
			b->refcnt = 1;
			release(&block_cache.lock);
			acquiresleep(&b->lock);
			return b;
		}
	}
	panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct block_buffer *
block_read(dev_t dev, uint64_t blockno) __acquires(&b->lock)
{
	struct block_buffer *b = block_get(dev, blockno);

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

	acquire(&block_cache.lock);
	b->refcnt--;
	if (b->refcnt == 0) {
		b->next->prev = b->prev;
		b->prev->next = b->next;
		b->next = block_cache.head.next;
		b->prev = &block_cache.head;
		block_cache.head.next->prev = b;
		block_cache.head.next = b;
	}
	release(&block_cache.lock);
}
