// Simple PIO-based (non-DMA) IDE driver code.

#include "buf.h"
#include "console.h"
#include "file.h"
#include "fs.h"
#include "ioapic.h"
#include "lib/compiler_attributes.h"
#include "mman.h"
#include "param.h"
#include "proc.h"
#include "sleeplock.h"
#include "spinlock.h"
#include "traps.h"
#include "x86.h"

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
static struct block_buffer *idequeue;

static int havedisk1;
static void idestart(struct block_buffer *);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr)
{
	int r;

	while (((r = inb(0x1f7)) & (IDE_BSY | IDE_DRDY)) != IDE_DRDY)
		;
	if (checkerr && (r & (IDE_DF | IDE_ERR)) != 0) {
		return -1;
	}
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
ide_disk_init(void)
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

	// Switch back to disk 0.
	outb(0x1f6, 0xe0 | (0 << 4));
}

void
ide_init(void)
{
	devsw[DEV_SD].open = ideopen;
	devsw[DEV_SD].close = ideclose;
	devsw[DEV_SD].read = ideread;
	devsw[DEV_SD].write = idewrite;
	devsw[DEV_SD].mmap = idemmap;
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct block_buffer *b)
{
	if (__unlikely(b == NULL)) {
		panic("idestart");
	}
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
	struct block_buffer *b;

	// First queued buffer is the active request.
	acquire(&idelock);

	if ((b = idequeue) == NULL) {
		release(&idelock);
		return;
	}
	idequeue = b->qnext;

	// Read data if needed.
	if (!(b->flags & B_DIRTY) && idewait(1) >= 0) {
		insl(0x1f0, b->data, BSIZE / 4);
	}

	// Wake process waiting for this buf.
	b->flags |= B_VALID;
	b->flags &= ~B_DIRTY;
	wakeup(b);

	// Start disk on next buf in queue.
	if (idequeue != NULL) {
		idestart(idequeue);
	}

	release(&idelock);
}

// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct block_buffer *b)
{
	struct block_buffer **pp;

	if (!holdingsleep(&b->lock)) {
		panic("iderw: buf not locked");
	}
	if ((b->flags & (B_VALID | B_DIRTY)) == B_VALID) {
		panic("iderw: nothing to do");
	}
	if (b->dev != 0 && !havedisk1) {
		panic("iderw: ide disk 1 not present");
	}

	acquire(&idelock); // DOC:acquire-lock

	// Append b to idequeue.
	b->qnext = NULL;
	for (pp = &idequeue; *pp; pp = &(*pp)->qnext) // DOC:insert-queue
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
