#include "log.h"
#include "bio.h"
#include "buf.h"
#include "console.h"
#include "fs.h"
#include "param.h"
#include "proc.h"
#include "spinlock.h"
#include <string.h>

// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
	size_t n;
	uintptr_t block[LOGSIZE];
};

struct log {
	struct spinlock lock;
	uintptr_t start;
	size_t size;
	size_t outstanding; // how many FS sys calls are executing.
	int committing; // in commit(), please wait.
	dev_t dev;
	struct logheader lh;
};
static struct log log;

static void recover_from_log(void);
static void commit(void);

void
initlog(dev_t dev)
{
	if (sizeof(struct logheader) >= BSIZE) {
		panic("initlog: too big logheader");
	}

	struct superblock sb;
	initlock(&log.lock, "log");
	read_superblock(dev, &sb);
	log.start = sb.logstart;
	log.size = sb.nlog;
	log.dev = dev;
	recover_from_log();
}

// Copy committed blocks from log to their home location
static void
install_trans(void)
{
	for (size_t tail = 0; tail < log.lh.n; tail++) {
		struct block_buffer *lbuf =
			block_read(log.dev, log.start + tail + 1); // read log block
		struct block_buffer *dbuf = block_read(log.dev, log.lh.block[tail]); // read
		                                                                     // dst
		memmove(dbuf->data, lbuf->data, BSIZE); // copy block to dst
		block_write(dbuf); // write dst to disk
		block_release(lbuf);
		block_release(dbuf);
	}
}

// Read the log header from disk into the in-memory log header
static void
read_head(void)
{
	struct block_buffer *buf = block_read(log.dev, log.start);
	struct logheader *lh = (struct logheader *)(buf->data);
	log.lh.n = lh->n;
	for (size_t i = 0; i < log.lh.n; i++) {
		log.lh.block[i] = lh->block[i];
	}
	block_release(buf);
}

// Write in-memory log header to disk.
// This is the true point at which the
// current transaction commits.
static void
write_head(void)
{
	struct block_buffer *buf = block_read(log.dev, log.start);
	struct logheader *hb = (struct logheader *)(buf->data);
	hb->n = log.lh.n;
	for (size_t i = 0; i < log.lh.n; i++) {
		hb->block[i] = log.lh.block[i];
	}
	block_write(buf);
	block_release(buf);
}

static void
recover_from_log(void)
{
	read_head();
	install_trans(); // if committed, copy from log to disk
	log.lh.n = 0;
	write_head(); // clear the log
}

// called at the start of each FS system call.
void
begin_op(void)
{
	acquire(&log.lock);
	while (1) {
		if (log.committing) {
			sleep(&log, &log.lock);
		} else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS > LOGSIZE) {
			// this op might exhaust log space; wait for commit.
			sleep(&log, &log.lock);
		} else {
			log.outstanding += 1;
			release(&log.lock);
			break;
		}
	}
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
	int do_commit = 0;

	acquire(&log.lock);
	log.outstanding -= 1;
	if (log.committing) {
		panic("log.committing");
	}
	if (log.outstanding == 0) {
		do_commit = 1;
		log.committing = 1;
	} else {
		// begin_op() may be waiting for log space,
		// and decrementing log.outstanding has decreased
		// the amount of reserved space.
		wakeup(&log);
	}
	release(&log.lock);

	if (do_commit) {
		// call commit w/o holding locks, since not allowed
		// to sleep with locks.
		commit();
		acquire(&log.lock);
		log.committing = 0;
		wakeup(&log);
		release(&log.lock);
	}
}

// Copy modified blocks from cache to log.
static void
write_log(void)
{
	for (size_t tail = 0; tail < log.lh.n; tail++) {
		struct block_buffer *to =
			block_read(log.dev, log.start + tail + 1); // log block
		struct block_buffer *from =
			block_read(log.dev, log.lh.block[tail]); // cache block
		memmove(to->data, from->data, BSIZE);
		block_write(to); // write the log
		block_release(from);
		block_release(to);
	}
}

static void
commit(void)
{
	if (log.lh.n > 0) {
		write_log(); // Write modified blocks from cache to log
		write_head(); // Write header to disk -- the real commit
		install_trans(); // Now install writes to home locations
		log.lh.n = 0;
		write_head(); // Erase the transaction from the log
	}
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache with B_DIRTY.
// commit()/write_log() will do the disk write.
//
// log_write() replaces block_write(); a typical use is:
//   bp = block_read(...)
//   modify bp->data[]
//   log_write(bp)
//   block_release(bp)
void
log_write(struct block_buffer *b)
{
	size_t i;

	if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1) {
		panic("too big a transaction");
	}
	if (log.outstanding < 1) {
		panic("log_write outside of trans");
	}

	acquire(&log.lock);
	for (i = 0; i < log.lh.n; i++) {
		if (log.lh.block[i] == b->blockno) { // log absorbtion
			break;
		}
	}
	log.lh.block[i] = b->blockno;
	if (i == log.lh.n) {
		log.lh.n++;
	}
	b->flags |= B_DIRTY; // prevent eviction
	release(&log.lock);
}
