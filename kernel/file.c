//
// File descriptors
//

#include "fs.h"
#include "include/sleeplock.h"
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include "param.h"
#include "spinlock.h"
#include "file.h"
#include "console.h"
#include "pipe.h"
#include "log.h"
#include "proc.h"
#include "lseek.h"

struct devsw devsw[NDEV];
struct {
	struct spinlock lock;
	struct file file[NFILE];
} ftable;

struct file *
fd_to_struct_file(int fd)
{
	return myproc()->ofile[fd];
}
void
fileinit(void)
{
	initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file *
filealloc(void)
{
	struct file *f;

	acquire(&ftable.lock);
	for (f = ftable.file; f < ftable.file + NFILE; f++) {
		if (f->ref == 0) {
			f->ref = 1;
			release(&ftable.lock);
			return f;
		}
	}
	release(&ftable.lock);
	return 0;
}

// Increment ref count for file f.
struct file *
filedup(struct file *f)
{
	acquire(&ftable.lock);
	if (unlikely(f->ref < 1))
		panic("filedup");
	f->ref++;
	release(&ftable.lock);
	return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
	struct file ff;

	acquire(&ftable.lock);
	if (unlikely(f->ref < 1))
		panic("fileclose");
	if (--f->ref > 0) {
		release(&ftable.lock);
		return;
	}
	if (S_ISBLK(f->ip->mode)) {
		if (f->ip->major < 0 || f->ip->major >= NDEV || !devsw[f->ip->major].open) {
			inode_unlockput(f->ip);
			end_op();
		}
		// Run device-specific opening code, if any.
		devsw[f->ip->major].close();
	}
	ff = *f;
	f->ref = 0;
	f->type = FD_NONE;
	release(&ftable.lock);

	if (ff.type == FD_PIPE)
		pipeclose(ff.pipe, ff.writable);
	else if (ff.type == FD_INODE) {
		begin_op();
		inode_put(ff.ip);
		end_op();
	}
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
	if (f->type == FD_INODE) {
		inode_lock(f->ip);
		inode_stat(f->ip, st);
		inode_unlock(f->ip);
		return 0;
	}
	return -ENOENT;
}

// Read from file f.
ssize_t
fileread(struct file *f, char *addr, uint64_t n)
{
	ssize_t r;

	if (f->readable == 0)
		return -EINVAL;
	if (f->type == FD_PIPE)
		return piperead(f->pipe, addr, n);
	if (f->type == FD_INODE) {
		inode_lock(f->ip);
		if ((r = inode_read(f->ip, addr, f->off, n)) > 0)
			f->off += r;
		inode_unlock(f->ip);
		return r;
	}
	panic("fileread");
}

off_t
fileseek(struct file *f, off_t n, int whence)
{
	off_t offset = 0;
	if (whence == SEEK_CUR) {
		offset = f->off + n;
	} else if (whence == SEEK_SET) {
		offset = n;
	} else if (whence == SEEK_END) {
		// Not sure if this is right.
		offset = f->ip->size;
	} else {
		return -EINVAL;
	}
	f->off = offset;
	return 0;
}

// Write to file f.
ssize_t
filewrite(struct file *f, char *addr, uint64_t n)
{
	ssize_t r;

	if (f->writable == 0)
		return -EROFS;
	if (f->type == FD_PIPE)
		return pipewrite(f->pipe, addr, n);
	if (f->type == FD_INODE) {
		// write a few blocks at a time to avoid exceeding
		// the maximum log transaction size, including
		// i-node, indirect block, allocation blocks,
		// and 2 blocks of slop for non-aligned writes.
		// this really belongs lower down, since inode_write()
		// might be writing a device like the console.
		size_t max = ((MAXOPBLOCKS - 1 - 1 - 2) / 2) * 512;
		size_t i = 0;
		while (i < n) {
			size_t n1 = n - i;
			if (n1 > max)
				n1 = max;

			begin_op();
			inode_lock(f->ip);
			if ((r = inode_write(f->ip, addr + i, f->off, n1)) > 0)
				f->off += r;
			inode_unlock(f->ip);
			end_op();

			if (r < 0)
				break;
			if (r != n1)
				panic("short filewrite");
			i += r;
		}
		return i == n ? n : -EDOM;
	}
	panic("filewrite");
}

static int
name_of_inode(struct inode *ip, struct inode *parent, char buf[static DIRSIZ], size_t n)
{
	off_t off;
	struct dirent de;
	for (off = 0; off < parent->size; off += sizeof(de)) {
		if (inode_read(parent, (char *)&de, off, sizeof(de)) != sizeof(de))
			panic("name_of_inode: can't read dir entry");
		if (de.d_ino == ip->inum) {
			strncpy(buf, de.d_name, n-1);
			return 0;
		}
	}
	return -1;
}


// Write n bytes to buf, including the null terminator.
// Returns the number of bytes written.
char *
inode_to_path(char *buf, size_t n, struct inode *ip)
{
	struct inode *parent;
	char node_name[DIRSIZ];
	bool isroot, isdir;
	inode_lock(ip);
	isroot = ip->inum == namei("/")->inum;
	isdir = S_ISDIR(ip->mode);
	inode_unlock(ip);

	if (isroot) {
		buf[0] = '/';
		buf[1] = '\0';
		return buf;
	} else if (isdir) {
		inode_lock(ip);
		parent = dirlookup(ip, "..", 0);
		inode_unlock(ip);
		inode_lock(parent);
		if (name_of_inode(ip, parent, node_name, n) < 0) {
			return NULL;
		}
		inode_unlock(parent);
		char *s = inode_to_path(buf, n, parent);
		if (strcmp(s, "/") != 0) {
			strncat(s, "/", 2);
		}
		return strncat(s, node_name, DIRSIZ - strlen(node_name));
	} else {
		return NULL;
	}
}
