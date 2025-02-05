// File system implementation.	Five layers:
//	 + Blocks: allocator for raw disk blocks.
//	 + Log: crash recovery for multi-step updates.
//	 + Files: inode allocator, reading, writing, metadata.
//	 + Directories: inode with special contents (list of other inodes!)
//	 + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.	The (higher-level) system call implementations
// are in sysfile.c.

#include "sleeplock.h"
#include <stdint.h>
#include <stat.h>
#include <dirent.h>
#include <date.h>
#include <time.h>
#include <errno.h>
#include "fs.h"
#include "param.h"
#include "proc.h"
#include "spinlock.h"
#include "buf.h"
#include "file.h"
#include "kernel_string.h"
#include "kernel_assert.h"
#include "bio.h"
#include "log.h"
#include "console.h"
#include "drivers/lapic.h"
#include "macros.h"

static void
inode_truncate(struct inode *);
// there should be one superblock per disk device, but we run with
// only one device
struct superblock global_sb;

// Read the super block.
void
read_superblock(int dev, struct superblock *sb)
{
	struct buf *bp;

	bp = block_read(dev, 1);
	memmove(sb, bp->data, sizeof(*sb));
	block_release(bp);
}

// Zero a block.
static void
bzero(int dev, int bno)
{
	struct buf *bp;

	bp = block_read(dev, bno);
	memset(bp->data, 0, BSIZE);
	log_write(bp);
	block_release(bp);
}

// Blocks.

// Allocate a zeroed disk block.
static uint32_t
block_alloc(uint32_t dev)
{
	int b, bi, m;
	struct buf *bp;

	bp = 0;
	for (b = 0; b < global_sb.size; b += BPB) {
		bp = block_read(dev, BBLOCK(b, global_sb));
		for (bi = 0; bi < BPB && b + bi < global_sb.size; bi++) {
			m = 1 << (bi % 8);
			if ((bp->data[bi / 8] & m) == 0) { // Is block free?
				bp->data[bi / 8] |= m; // Mark block in use.
				log_write(bp);
				block_release(bp);
				bzero(dev, b + bi);
				return b + bi;
			}
		}
		block_release(bp);
	}
	panic("block_alloc: out of blocks");
}

// Free a disk block.
static void
block_free(int dev, uint32_t b)
{
	struct buf *bp;
	int bi, m;

	bp = block_read(dev, BBLOCK(b, global_sb));
	bi = b % BPB;
	m = 1 << (bi % 8);
	if ((bp->data[bi / 8] & m) == 0)
		panic("freeing free block");
	bp->data[bi / 8] &= ~m;
	log_write(bp);
	block_release(bp);
}

// Inodes.
//
// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at
// global_sb.startinode. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a cache of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The cached
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//	 is non-zero. inode_alloc() allocates, and inode_put() frees if
//	 the reference and link counts have fallen to zero.
//
// * Referencing in cache: an entry in the inode cache
//	 is free if ip->ref is zero. Otherwise ip->ref tracks
//	 the number of in-memory pointers to the entry (open
//	 files and current directories). inode_get() finds or
//	 creates a cache entry and increments its ref; inode_put()
//	 decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//	 cache entry is only correct when ip->valid is 1.
//	 inode_lock() reads the inode from
//	 the disk and sets ip->valid, while inode_put() clears
//	 ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//	 the information in an inode and its content if it
//	 has first locked the inode.
//
// Thus a typical sequence is:
//	 ip = inode_get(dev, inum)
//	 inode_lock(ip)
//	 ... examine and modify ip->xxx ...
//	 inode_unlock(ip)
//	 inode_put(ip)
//
// inode_lock() is separate from inode_get() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. inode_get() increments ip->ref so that the inode
// stays cached and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The inode_cache.lock spin-lock protects the allocation of icache
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold inode_cache.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

#define NBUCKET NCPU
struct {
	struct spinlock lock[NBUCKET];
	struct inode inode[NINODE][NBUCKET];
} inode_cache;

void
inode_init(int dev)
{
	for (int i = 0; i < min(NBUCKET, ncpu); i++) {
		initlock(&inode_cache.lock[i], "inode.bucket");
	}
	//initlock(&inode_cache.lock, "icache");
	for (int j = 0; j < min(NBUCKET, ncpu); j++) {
		for (int i = 0; i < NINODE; i++) {
			initsleeplock(&inode_cache.inode[i][j].lock, "inode");
		}
	}

	read_superblock(dev, &global_sb);
	cprintf("sb: size %lu nblocks %lu ninodes %lu nlog %lu logstart %lu\
 inodestart %lu bmap start %lu\n",
					global_sb.size, global_sb.nblocks, global_sb.ninodes, global_sb.nlog,
					global_sb.logstart, global_sb.inodestart, global_sb.bmapstart);
}

static struct inode *
inode_get(uint32_t dev, uint32_t inum);

// Allocate an inode on device dev.
// Mark it as allocated by giving it type type.
// Returns an unlocked but allocated and referenced inode.
struct inode *
inode_alloc(uint32_t dev, int mode)
{
	int inum;
	struct buf *bp;
	struct dinode *dip;

	for (inum = 1; inum < global_sb.ninodes; inum++) {
		bp = block_read(dev, IBLOCK(inum, global_sb));
		dip = (struct dinode *)bp->data + inum % IPB;
		if (!S_ISANY(dip->mode)) { // a free inode
			memset(dip, 0, sizeof(*dip));
			dip->mode = mode;
			dip->gid = DEFAULT_GID;
			dip->uid = DEFAULT_UID;
			struct rtcdate rtc;
			cmostime(&rtc);
			dip->ctime = RTC_TO_UNIX(rtc);
			dip->atime = RTC_TO_UNIX(rtc);
			dip->mtime = RTC_TO_UNIX(rtc);
			log_write(bp); // mark it allocated on the disk
			block_release(bp);
			return inode_get(dev, inum);
		}
		block_release(bp);
	}
	panic("inode_alloc: no inodes");
}

// Copy a modified in-memory inode to disk.
// Must be called after every change to an ip->xxx field
// that lives on disk, since i-node cache is write-through.
// Caller must hold ip->lock.
void
inode_update(struct inode *ip)
{
	struct buf *bp;
	struct dinode *dip;

	bp = block_read(ip->dev, IBLOCK(ip->inum, global_sb));
	dip = (struct dinode *)bp->data + ip->inum % IPB;
	dip->major = ip->major;
	dip->minor = ip->minor;
	dip->nlink = ip->nlink;
	dip->size = ip->size;
	dip->mode = ip->mode;
	struct rtcdate rtc;
	cmostime(&rtc);
	dip->mtime = RTC_TO_UNIX(rtc);
	dip->atime = RTC_TO_UNIX(rtc);
	dip->ctime = ip->ctime;

	memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
	log_write(bp);
	block_release(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode *
inode_get(uint32_t dev, uint32_t inum)
{
	struct inode *ip, *empty;

	int hash = dev % min(NBUCKET, ncpu);
	acquire(&inode_cache.lock[hash]);

	// Is the inode already cached?
	empty = 0;
	for (ip = &inode_cache.inode[0][hash];
			 ip < &inode_cache.inode[NINODE - 1][hash];
			 ip++) {
		if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
			ip->ref++;
			release(&inode_cache.lock[hash]);
			return ip;
		}
		if (empty == 0 && ip->ref == 0) // Remember empty slot.
			empty = ip;
	}
	if (empty == 0) {
		for (int i = 0; i < min(NBUCKET, ncpu); i++) {
			// Skip over the node that we already checked.
			// This also avoids a deadlock.
			if (i == hash)
				continue;
			acquire(&inode_cache.lock[i]);
			for (ip = &inode_cache.inode[0][i]; ip < &inode_cache.inode[NINODE - 1][i];
			  	 ip++) {

				if (ip->ref > 0 && ip->dev == dev && ip->inum == inum) {
					ip->ref++;
					release(&inode_cache.lock[i]);
					return ip;
				}
				if (empty == 0 && ip->ref == 0) // Remember empty slot.
					empty = ip;
			}
			release(&inode_cache.lock[i]);
		}
	}

	// Recycle an inode cache entry.
	if (empty == 0)
		panic("inode_get: no inodes");

	ip = empty;
	ip->dev = dev;
	ip->inum = inum;
	ip->ref = 1;
	ip->valid = 0;
	release(&inode_cache.lock[hash]);

	return ip;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode *
inode_dup(struct inode *ip)
{
	__sync_add_and_fetch(&ip->ref, 1);
	//acquire(&inode_cache.lock);
	//ip->ref++;
	//release(&inode_cache.lock);
	return ip;
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void
inode_lock(struct inode *ip)
{
	struct buf *bp;
	struct dinode *dip;

	if (ip == 0 || ip->ref < 1)
		panic("inode_lock");
	kernel_assert(!holdingsleep(&ip->lock));

	acquiresleep(&ip->lock);

	if (ip->valid == 0) {
		bp = block_read(ip->dev, IBLOCK(ip->inum, global_sb));
		dip = (struct dinode *)bp->data + ip->inum % IPB;
		ip->major = dip->major;
		ip->minor = dip->minor;
		ip->nlink = dip->nlink;
		ip->size = dip->size;
		ip->mode = dip->mode;
		ip->uid = dip->uid;
		ip->gid = dip->gid;
		ip->atime = dip->atime;
		ip->ctime = dip->ctime;
		ip->mtime = dip->mtime;

		memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
		block_release(bp);
		ip->valid = 1;
		if (ip->mode == 0)
			panic("inode_lock: no mode");
	}
}

// Unlock the given inode.
void
inode_unlock(struct inode *ip)
{
	if (ip == 0 || ip->ref < 1)
		panic("inode_unlock");
	kernel_assert(holdingsleep(&ip->lock));

	releasesleep(&ip->lock);
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode cache entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to inode_put() must be inside a transaction in
// case it has to free the inode.
void
inode_put(struct inode *ip)
{
	acquiresleep(&ip->lock);
	if (ip->valid && ip->nlink == 0) {
		acquire(&inode_cache.lock[1]);
		int r = ip->ref;
		release(&inode_cache.lock[1]);
		if (r == 1) {
			// inode has no links and no other references: truncate and free.
			inode_truncate(ip);
			ip->mode = 0;
			inode_update(ip);
			ip->valid = 0;
		}
	}
	releasesleep(&ip->lock);

	//acquire(&inode_cache.lock);
	__sync_sub_and_fetch(&ip->ref, 1);
	//ip->ref--;
	//release(&inode_cache.lock);
}

// Common idiom: unlock, then put.
void
inode_unlockput(struct inode *ip)
{
	inode_unlock(ip);
	inode_put(ip);
}

// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static uint32_t
bmap(struct inode *ip, uint32_t bn)
{
	uint32_t addr, *a;
	struct buf *bp;

	kernel_assert(holdingsleep(&ip->lock));
	if (bn < NDIRECT) {
		if ((addr = ip->addrs[bn]) == 0)
			ip->addrs[bn] = addr = block_alloc(ip->dev);
		return addr;
	}
	bn -= NDIRECT;

	if (bn < NINDIRECT) {
		// Load indirect block, allocating if necessary.
		if ((addr = ip->addrs[NDIRECT]) == 0)
			ip->addrs[NDIRECT] = addr = block_alloc(ip->dev);
		bp = block_read(ip->dev, addr);
		a = (uint32_t *)bp->data;
		if ((addr = a[bn]) == 0) {
			a[bn] = addr = block_alloc(ip->dev);
			log_write(bp);
		}
		block_release(bp);
		return addr;
	}
	bn -= NINDIRECT;

	//if (bn >= NINDIRECT * NINDIRECT)
	//	bn -= NINDIRECT * NINDIRECT;
	if (bn < NINDIRECT * NINDIRECT) {
		// Load indirect block, allocating if necessary.
		if ((addr = ip->addrs[NDIRECT + 1]) == 0)
			ip->addrs[NDIRECT + 1] = addr = block_alloc(ip->dev);
		bp = block_read(ip->dev, addr);
		a = (uint32_t *)bp->data;
		uint32_t double_index = bn / NINDIRECT;

		if ((addr = a[double_index]) == 0) {
			a[double_index] = addr = block_alloc(ip->dev);
			log_write(bp);
		}
		block_release(bp);

		bp = block_read(ip->dev, addr);
		a = (uint32_t *)bp->data;
		uint32_t pos = bn % NINDIRECT;

		// load doubly indirect block
		if ((addr = a[pos]) == 0) {
			a[pos] = addr = block_alloc(ip->dev);
			log_write(bp);
		}
		block_release(bp);
		return addr;
	}

	panic("bmap: out of range");
}

// Truncate inode (discard contents).
// Only called when the inode has no links
// to it (no directory entries referring to it)
// and has no in-memory reference to it (is
// not an open file or current directory).
static void
inode_truncate(struct inode *ip)
{
	// NOTE: change the a and a2 pointers to uintptr_t
	// if/when BSIZE increases to 4096+
	struct buf *bp;
	uint32_t *a;

	for (int i = 0; i < NDIRECT; i++) {
		if (ip->addrs[i]) {
			block_free(ip->dev, ip->addrs[i]);
			ip->addrs[i] = 0;
		}
	}

	if (ip->addrs[NDIRECT]) {
		bp = block_read(ip->dev, ip->addrs[NDIRECT]);
		a = (uint32_t *)bp->data;
		for (int j = 0; j < NINDIRECT; j++) {
			if (a[j]) {
				block_free(ip->dev, a[j]);
			} else {
				// TODO: Is this a performance optimization?
				// Or does it limit file deletion functionality?
				break;
			}
		}
		block_release(bp);
		block_free(ip->dev, ip->addrs[NDIRECT]);
		ip->addrs[NDIRECT] = 0;
	}
	if (ip->addrs[NDIRECT + 1]) {
		// get doubly indirect block
		bp = block_read(ip->dev, ip->addrs[NDIRECT + 1]);
		a = (uint32_t *)bp->data;
		for (int i = 0; i < NINDIRECT; i++) {
			// get indirect block
			if (a[i]) {
				struct buf *bp2 = block_read(ip->dev, a[i]);
				uint32_t *a2 = (uint32_t *)bp2->data;
				for (int j = 0; j < NINDIRECT; j++) {
					// free block
					if (a2[j]) {
						block_free(ip->dev, a2[j]);
					} else {
						break;
					}
				}
				// free indirect block
				block_release(bp2);
				block_free(ip->dev, a[i]);
			}
		}
		block_release(bp);
		block_free(ip->dev, ip->addrs[NDIRECT + 1]);
		ip->addrs[NDIRECT + 1] = 0;
	}

	ip->size = 0;
	inode_update(ip);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void
inode_stat(struct inode *ip, struct stat *st)
{
	kernel_assert(holdingsleep(&ip->lock));
	st->st_dev = ip->dev;
	st->st_ino = ip->inum;
	st->st_nlink = ip->nlink;
	st->st_size = ip->size;
	st->st_mode = ip->mode;
	st->st_uid = ip->uid;
	st->st_gid = ip->gid;
	st->st_atime = ip->atime;
	st->st_ctime = ip->ctime;
	st->st_mtime = ip->mtime;
}

// Read data from inode.
// Caller must hold ip->lock.
int
inode_read(struct inode *ip, char *dst, uint64_t off, uint64_t n)
{
	kernel_assert(holdingsleep(&ip->lock));
	uint64_t tot, m;
	struct buf *bp;

	if (S_ISBLK(ip->mode)) {
		if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
			return -ENODEV;
		return devsw[ip->major].read(ip, dst, n);
	}

	if (off > ip->size || off + n < off)
		return -EDOM;
	if (off + n > ip->size)
		n = ip->size - off;

	for (tot = 0; tot < n; tot += m, off += m, dst += m) {
		bp = block_read(ip->dev, bmap(ip, off / BSIZE));
		m = min(n - tot, BSIZE - off % BSIZE);
		memmove(dst, bp->data + off % BSIZE, m);
		block_release(bp);
	}
	return n;
}

// Write data to inode.
// Caller must hold ip->lock.
int
inode_write(struct inode *ip, char *src, uint64_t off, uint64_t n)
{
	kernel_assert(holdingsleep(&ip->lock));
	uint64_t tot, m;
	struct buf *bp;

	if (S_ISBLK(ip->mode)) {
		if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
			return -ENODEV;
		return devsw[ip->major].write(ip, src, n);
	}

	if (off > ip->size || off + n < off)
		return -EDOM;
	if (off + n > MAXFILE * BSIZE)
		return -EDOM;

	for (tot = 0; tot < n; tot += m, off += m, src += m) {
		bp = block_read(ip->dev, bmap(ip, off / BSIZE));
		m = min(n - tot, BSIZE - off % BSIZE);
		memmove(bp->data + off % BSIZE, src, m);
		log_write(bp);
		block_release(bp);
	}

	if (n > 0 && off > ip->size) {
		ip->size = off;
		inode_update(ip);
	}
	return n;
}

// Directories

int
namecmp(const char *s, const char *t)
{
	return strncmp(s, t, DIRSIZ);
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode *
dirlookup(struct inode *dp, const char *name, uint64_t *poff)
{
	uint64_t off, inum;
	struct dirent de;

	if (!S_ISDIR(dp->mode))
		panic("dirlookup not DIR");

	for (off = 0; off < dp->size; off += sizeof(de)) {
		if (inode_read(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
			panic("dirlookup read");
		if (de.inum == 0)
			continue;
		if (namecmp(name, de.name) == 0) {
			// entry matches path element
			if (poff)
				*poff = off;
			inum = de.inum;
			return inode_get(dp->dev, inum);
		}
	}

	return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
static uint32_t last_inum = 0;
static uint64_t last_offset_from_inum = 0;
int
dirlink(struct inode *dp, const char *name, uint32_t inum)
{
	uint64_t off = 0;
	struct dirent de;
	struct inode *ip;

	// Check that name is not present.
	if ((ip = dirlookup(dp, name, 0)) != 0) {
		inode_put(ip);
		return -1;
	}
	if (inum == last_inum) {
		if (last_offset_from_inum + sizeof(de) < dp->size) {
			off = last_offset_from_inum + sizeof(de);
		} else {
			off = 0;
		}
	}
	// Look for an empty dirent.
	for (; off < dp->size; off += sizeof(de)) {
		if (inode_read(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
			panic("dirlink read");
		if (de.inum == 0) {
			last_offset_from_inum = off;
			break;
		}
	}

	strncpy(de.name, name, DIRSIZ);
	de.inum = inum;
	if (inode_write(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
		panic("dirlink");
	last_inum = inum;

	return 0;
}

// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//	 skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//	 skipelem("///a//bb", name) = "bb", setting name = "a"
//	 skipelem("a", name) = "", setting name = "a"
//	 skipelem("", name) = skipelem("////", name) = 0
//
static const char *
skipelem(const char *path, char *name)
{
	const char *s;
	size_t len;

	while (*path == '/')
		path++;
	if (*path == 0)
		return 0;
	s = path;
	while (*path != '/' && *path != 0)
		path++;
	len = path - s;
	if (len >= DIRSIZ)
		memmove(name, s, DIRSIZ);
	else {
		memmove(name, s, len);
		name[len] = 0;
	}
	while (*path == '/')
		path++;
	return path;
}

// Look up and return the inode for a path name.
// If nameiparent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls inode_put().
// iow, path -> inode.
static struct inode *
namex(const char *path, int nameiparent, char *name)
{
	struct inode *ip, *next;

	if (*path == '/')
		ip = inode_get(ROOTDEV, ROOTINO);
	else
		ip = inode_dup(myproc()->cwd); // increase refcount

	while ((path = skipelem(path, name)) != 0) {
		inode_lock(ip);
		if (!S_ISDIR(ip->mode)) {
			inode_unlockput(ip);
			return 0;
		}
		if (nameiparent && *path == '\0') {
			// Stop one level early.
			inode_unlock(ip);
			return ip;
		}
		if ((next = dirlookup(ip, name, 0)) == 0) {
			inode_unlockput(ip);
			return 0;
		}
		inode_unlockput(ip);
		ip = next;
	}
	if (nameiparent) {
		inode_put(ip);
		return 0;
	}
	return ip;
}

struct inode *
namei(const char *path)
{
	char name[DIRSIZ];
	return namex(path, 0, name);
}

struct inode *
nameiparent(const char *path, char *name)
{
	return namex(path, 1, name);
}
