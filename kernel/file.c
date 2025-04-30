//
// File descriptors
//

#include "fs.h"
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include "param.h"
#include "spinlock.h"
#include "file.h"
#include "console.h"
#include "pipe.h"
#include "lib/ring_buffer.h"
#include "log.h"
#include "proc.h"
#include "fcntl_constants.h"
#include "lseek.h"
#include <stdbool.h>

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
			f->flags = 0;
			release(&ftable.lock);
			return f;
		}
	}
	release(&ftable.lock);
	return NULL;
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

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
int
fdalloc(struct file *f)
{
	int fd;
	struct proc *curproc = myproc();

	for (fd = 0; fd < NOFILE; fd++) {
		if (curproc->ofile[fd] == NULL) {
			curproc->ofile[fd] = f;
			return fd;
		}
	}
	return -EMFILE;
}

// Follows a symbolic link until we either resolve it or recurse too much.
// Caller must hold lock.
// On success, we return a new locked inode, unlocking the first one.
// On failure, we return NULL, and the inode is no longer locked.
struct inode *
link_dereference(struct inode *ip, char *buff) __must_hold(&ip->lock)
{
	int ref_count = NLINK_DEREF;
	struct inode *new_ip = ip;
	while (S_ISLNK(new_ip->mode)) {
		ref_count--;
		if (ref_count == 0)
			goto bad;

		if (inode_read(new_ip, buff, 0, new_ip->size) < 0)
			goto bad;

		inode_unlock(new_ip);

		if ((new_ip = namei(buff)) == NULL)
			goto bad;
		inode_lock(new_ip);
	}
	return new_ip;
bad:
	inode_unlock(new_ip);
	return NULL;
}

// Holds lock on ip when released.
struct inode *
filecreate(char *path, mode_t mode, short major, short minor)
{
	struct inode *ip, *dp;
	char name[DIRSIZ];

	// POSIX says that creat(2) makes an IFREG if IFMT is 0.
	if ((mode & S_IFMT) == 0)
		mode |= S_IFREG;

	// get inode of path, and put the name in name.
	if ((dp = nameiparent(path, name)) == NULL)
		return NULL;
	inode_lock(dp);

	// /dp/name is present
	if ((ip = dirlookup(dp, name, NULL)) != NULL) {
		inode_unlockput(dp);
		inode_lock(ip);
		if (S_ISREG(ip->mode) && S_ISREG(mode)) {
			ip->mode = mode;
			return ip;
		}
		if (S_ISLNK(ip->mode) && S_ISLNK(mode)) {
			ip->mode = mode;
			return ip;
		}
		if (S_ISFIFO(ip->mode) && S_ISFIFO(mode)) {
			ip->mode = mode;
			return ip;
		}
		inode_unlockput(ip);
		return NULL;
	}

	if ((ip = inode_alloc(dp->dev, mode)) == NULL)
		panic("create: inode_alloc");

	inode_lock(ip);
	ip->major = major;
	ip->minor = minor;
	ip->nlink = 1;
	ip->mode = mode;
	ip->gid = DEFAULT_GID;
	ip->uid = DEFAULT_UID;
	ip->flags = 0;
	// atime, mtime, etc. get handled in inode_update()
	inode_update(ip);
	// Create . and .. entries.
	// because every directory goes as follows:
	// $ ls -l
	// .
	// ..
	// dir/
	// $ cd dir
	// $ ls -l
	// .
	// ..
	if (S_ISDIR(mode)) {
		dp->nlink++; // for ".."
		inode_update(dp);
		// No ip->nlink++ for ".": avoid cyclic ref count.
		if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
			panic("create dots");
	}

	if (dirlink(dp, name, ip->inum) < 0)
		panic("create: dirlink");

	inode_unlockput(dp);

	return ip;
}

int
fileopen(char *path, int flags, mode_t mode)
{
	int fd;
	struct file *f;
	struct inode *ip;

	if (path == NULL)
		return -EFAULT;

	begin_op();

	if ((flags & O_CREATE) == O_CREATE) {
		// try to create a file and it exists.
		if ((ip = namei(path)) != NULL) {
			// if it's a char device, possibly do something special.
			inode_lock(ip);
			if (S_ISCHR(ip->mode)) {
				goto get_fd;
			}
			// if it's not a char device, just exit.
			inode_unlockput(ip);
			end_op();
			return -EEXIST;
		}
		// filecreate() holds a lock on this inode pointer,
		// but only if it succeeds.
		ip = filecreate(path, mode, 0, 0);
		if (ip == NULL) {
			end_op();
			return -EIO;
		}
	} else {
		if ((ip = namei(path)) == NULL) {
			end_op();
			return -ENOENT;
		}
		inode_lock(ip);
		ip->flags = flags;

		if (S_ISLNK(ip->mode)) {
			if ((ip = link_dereference(ip, path)) == NULL) {
				inode_unlockput(ip);
				end_op();
				return -EINVAL;
			}
		}
		if (S_ISDIR(ip->mode) && ((flags & O_ACCMODE) != O_RDONLY)) {
			inode_unlockput(ip);
			end_op();
			return -EISDIR;
		}
		if (S_ISCHR(ip->mode)) {
			if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].open) {
				inode_unlockput(ip);
				end_op();
				return -ENODEV;
			}
			// Run device-specific opening code, if any.
			devsw[ip->major].open(ip->minor, flags);
		}
	}
	if (S_ISFIFO(ip->mode) && (flags & O_NONBLOCK) != O_NONBLOCK) {
		if (ip->rf == NULL && ip->wf == NULL) {
			if (pipealloc(&ip->rf, &ip->wf) < 0) {
				inode_unlockput(ip);
				end_op();
				return -1;
			}
			ip->rf->type = FD_FIFO;
			ip->wf->type = FD_FIFO;
			ip->rf->ip = ip;
			ip->wf->ip = ip;

			// As explained in pipealloc, writing to one pipe
			// affects the other one because they are pointing
			// to the same one. For example, writing to rf's
			// pipe changes wf's pipe.
			ip->rf->pipe->writeopen = 0;
			ip->rf->pipe->readopen = 0;
		}
		if ((flags & O_ACCMODE) == O_WRONLY) {
			// O_WRONLY is special for FIFOs.
			// If there is no reader open, then we
			// have to exit with this errno value.
			if (ip->wf->pipe->readopen == 0) {
				inode_unlock(ip);
				end_op();
				return -ENXIO;
			}

			ip->wf->pipe->writeopen++;
			ip->wf->ref++;

			if ((fd = fdalloc(ip->wf)) < 0) {
				inode_unlock(ip);
				end_op();
				return fd;
			}
			inode_unlock(ip);
			acquire(&(ip->wf->pipe)->lock);
			while (!(ip->wf->pipe)->readopen) {
				wakeup(&(ip->wf->pipe)->ring_buffer->nread);
				sleep(&(ip->wf->pipe)->ring_buffer->nwrite, &(ip->wf->pipe)->lock);
			}
			wakeup(&(ip->wf->pipe)->ring_buffer->nread);
			release(&(ip->wf->pipe)->lock);

			end_op();
			return fd;

		} else if ((flags & O_ACCMODE) == O_RDONLY) {
			ip->rf->pipe->readopen++;
			ip->rf->ref++;
			if ((fd = fdalloc(ip->rf)) < 0) {
				inode_unlock(ip);
				end_op();
				return fd;
			}
			inode_unlock(ip);

			acquire(&(ip->rf->pipe)->lock);
			// Wait for a write side to be opened.
			// Skips around and goes into the scheduler to avoid spinning on this.
			while (!(ip->rf->pipe)->writeopen) {
				wakeup(&(ip->rf->pipe)->ring_buffer->nwrite);
				sleep(&(ip->rf->pipe)->ring_buffer->nread, &(ip->rf->pipe)->lock);
			}
			wakeup(&(ip->rf->pipe)->ring_buffer->nwrite);
			release(&(ip->rf->pipe)->lock);

			end_op();
			return fd;
		}
	}
	// By this line, both branches above are holding a lock to ip.
	// That is why it is released down here.
get_fd:

	if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0) {
		// Fileclose returns int but
		// we ignore it because we error out regardless.
		if (f)
			(void)fileclose(f);
		inode_unlockput(ip);
		end_op();
		return -EBADF;
	}
	inode_unlock(ip);
	end_op();

	f->type = FD_INODE;
	f->ip = ip;
	f->off = (flags & O_APPEND) ? f->ip->size : 0;
	f->readable = !((flags & O_ACCMODE) == O_WRONLY);
	f->writable = ((flags & O_ACCMODE) == O_WRONLY) ||
								((flags & O_ACCMODE) == O_RDWR);
	return fd;
}
// Close file f.  (Decrement ref count, close when reaches 0.)
int
fileclose(struct file *f)
{
	struct file ff;

	acquire(&ftable.lock);
	if (unlikely(f->ref < 1))
		panic("fileclose");
	// Is the file open somewhere else?
	// If so, just leave with success.
	if (--f->ref > 0) {
		release(&ftable.lock);
		return 0;
	}
	if (S_ISCHR(f->ip->mode)) {
		if (f->ip->major < 0 || f->ip->major >= NDEV || devsw[f->ip->major].close == NULL) {
			release(&ftable.lock);
			return -ENODEV;
		}
		// Run device-specific opening code, if any.
		devsw[f->ip->major].close(f->ip->minor);
	}
	ff = *f;
	f->ref = 0;
	f->type = FD_NONE;
	release(&ftable.lock);

	if (ff.type == FD_PIPE || ff.type == FD_FIFO) {
		pipeclose(ff.pipe, ff.writable);
		if (ff.type == FD_FIFO) {
			ff.ip->rf->ref = 0;
			ff.ip->wf->ref = 0;
		}
	} else if (ff.type == FD_INODE) {
		begin_op();
		inode_put(ff.ip);
		end_op();
	}
	return 0;
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
	if (f->type == FD_INODE || f->type == FD_FIFO) {
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
	if (f->type == FD_PIPE || f->type == FD_FIFO) {
		return piperead(f->pipe, addr, n);
	} else if (f->type == FD_INODE) {
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
	if (f->type == FD_PIPE || f->type == FD_FIFO)
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
name_of_inode(struct inode *ip, struct inode *parent, char buf[static DIRSIZ],
							size_t n)
{
	off_t off;
	struct dirent de;
	for (off = 0; off < parent->size; off += sizeof(de)) {
		if (inode_read(parent, (char *)&de, off, sizeof(de)) != sizeof(de))
			panic("name_of_inode: can't read dir entry");
		if (de.d_ino == ip->inum) {
			strncpy(buf, de.d_name, n - 1);
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
		parent = dirlookup(ip, "..", NULL);
		inode_unlock(ip);
		inode_lock(parent);
		if (name_of_inode(ip, parent, node_name, n) < 0) {
			inode_unlock(parent);
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
