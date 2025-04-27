// Simple PIO-based (non-DMA) IDE driver code.

#include "mman.h"
#include "param.h"
#include "proc.h"
#include "sleeplock.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "buf.h"
#include "ioapic.h"
#include "console.h"
#include "lib/compiler_attributes.h"

#define SECTOR_SIZE 512
#define IDE_BSY 0x80
#define IDE_DRDY 0x40
#define IDE_DF 0x20
#define IDE_ERR 0x01

#define IDE_CMD_READ 0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;

static int havedisk1;
static void
idestart(struct buf *);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr)
{
	int r;

	while (((r = inb(0x1f7)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY)
		;
	if (checkerr && (r & (IDE_DF | IDE_ERR)) != 0)
		return -1;
	return 0;
}

static int
ideopen(short minor, int flags)
{
	return 0;
}

static int
ideclose(short minor)
{
	return 0;
}

static ssize_t
ideread(short minor, struct inode *ip, char *buf, size_t len)
{
	return len;
}

static ssize_t
idewrite(short minor, struct inode *ip, char *buf, size_t len)
{
	return len;
}

static struct mmap_info
idemmap(short minor, size_t length, uintptr_t addr, int perm)
{
	return (struct mmap_info){};
}


__cold void
ideinit(void)
{

	initlock(&idelock, "ide");
	ioapicenable(IRQ_IDE, ncpu - 1);
	idewait(0);

	// Check if disk 1 is present
	outb(0x1f6, 0xe0 | (1 << 4));
	for (int i = 0; i < 1000; i++) {
		if (inb(0x1f7) != 0) {
			havedisk1 = 1;
			break;
		}
	}

	devsw[SD].open = ideopen;
	devsw[SD].close = ideclose;
	devsw[SD].read = ideread;
	devsw[SD].write = idewrite;
	devsw[SD].mmap = idemmap;

	// Switch back to disk 0.
	outb(0x1f6, 0xe0 | (0 << 4));
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
	if (unlikely(b == NULL))
		panic("idestart");
	if (b->blockno >= FSSIZE) {
		uart_printf("blockno: %ld\n", b->blockno);
		panic("incorrect blockno");
	}
	const int sector_per_block = BSIZE / SECTOR_SIZE;
	size_t sector = b->blockno * sector_per_block;
	int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ : IDE_CMD_RDMUL;
	int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

	idewait(0);
	outb(0x3f6, 0); // generate interrupt
	outb(0x1f2, sector_per_block); // number of sectors
	outb(0x1f3, sector & 0xff);
	outb(0x1f4, (sector >> 8) & 0xff);
	outb(0x1f5, (sector >> 16) & 0xff);
	outb(0x1f6, 0xe0 | ((b->dev & 1) << 4) | ((sector >> 24) & 0x0f));
	if (b->flags & B_DIRTY) {
		outb(0x1f7, write_cmd);
		outsl(0x1f0, b->data, BSIZE / 4);
	} else {
		outb(0x1f7, read_cmd);
	}
}

// Interrupt handler.
void
ideintr(void)
{
	struct buf *b;

	// First queued buffer is the active request.
	acquire(&idelock);

	if ((b = idequeue) == NULL) {
		release(&idelock);
		return;
	}
	idequeue = b->qnext;

	// Read data if needed.
	if (!(b->flags & B_DIRTY) && idewait(1) >= 0)
		insl(0x1f0, b->data, BSIZE / 4);

	// Wake process waiting for this buf.
	b->flags |= B_VALID;
	b->flags &= ~B_DIRTY;
	wakeup(b);

	// Start disk on next buf in queue.
	if (idequeue != NULL)
		idestart(idequeue);

	release(&idelock);
}

// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
	struct buf **pp;

	if (!holdingsleep(&b->lock))
		panic("iderw: buf not locked");
	if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID)
		panic("iderw: nothing to do");
	if (b->dev != 0 && !havedisk1)
		panic("iderw: ide disk 1 not present");

	acquire(&idelock); //DOC:acquire-lock

	// Append b to idequeue.
	b->qnext = NULL;
	for (pp = &idequeue; *pp; pp = &(*pp)->qnext) //DOC:insert-queue
		;
	*pp = b;

	// Start disk if necessary.
	if (idequeue == b) {
		idestart(b);
	}

	// Wait for request to finish.
	while ((b->flags & (B_VALID | B_DIRTY)) != B_VALID) {
		sleep(b, &idelock);
	}

	release(&idelock);
}


