// Fake IDE disk; stores blocks in memory.
// Useful for running kernel without scratch disk.

#include "sleeplock.h"
#include <stdint.h>
#include <buf.h>
#include "fs.h"
#include "console.h"
#include <string.h>

extern uint8_t _binary_fs_img_start[], _binary_fs_img_size[];

static size_t disksize;
static uint8_t *memdisk;

void
ideinit(void)
{
	memdisk = _binary_fs_img_start;
	disksize = (size_t)_binary_fs_img_size / BSIZE;
}

// Interrupt handler.
void
ideintr(void)
{
	// no-op
}

// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
	uint8_t *p;

	if (!holdingsleep(&b->lock))
		panic("iderw: buf not locked");
	if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
		panic("iderw: nothing to do");
	if (b->dev != 1)
		panic("iderw: request not for disk 1");
	if (b->blockno >= disksize)
		panic("iderw: block out of range");

	p = memdisk + b->blockno * BSIZE;

	if (b->flags & B_DIRTY) {
		b->flags &= ~B_DIRTY;
		memmove(p, b->data, BSIZE);
	} else
		memmove(b->data, p, BSIZE);
	b->flags |= B_VALID;
}
