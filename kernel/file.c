//
// File descriptors
//

#include "file.h"
#include "console.h"
#include "fs.h"
#include "kernel_assert.h"
#include "lib/ring_buffer.h"
#include "limits.h"
#include "log.h"
#include "param.h"
#include "pipe.h"
#include "proc.h"
#include "spinlock.h"
#include <bits/fcntl_constants.h>
#include <bits/seek_constants.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

struct devsw devsw[NDEV];

typedef struct {
	struct spinlock lock;
	struct file file[NFILE];
} file_table_t;

static file_table_t file_table;

struct file *
fd_to_struct_file(int fd)
{
	if (fd < 0 || fd >= OPEN_MAX) {
		return NULL;
	}
	return myproc()->ofile[fd];
}

void
fileinit(void)
{
	initlock(&file_table.lock, "ftable");
}

// Allocate a file structure.
struct file *
filealloc(void)
{
	acquire(&file_table.lock);

	for (struct file *f = file_table.file; f < file_table.file + NFILE; f++) {
		if (f->ref == 0) {
			f->ref = 1;
			f->flags = 0;
			release(&file_table.lock);
			return f;
		}
	}
	release(&file_table.lock);

	return NULL;
}

// Increment ref count for file f.
struct file *
filedup(struct file *f, int flags)
{
	acquire(&file_table.lock);

	if (__unlikely(f->ref < 1)) {
		panic("filedup");
	}
	f->ref++;
	f->flags = flags;
	release(&file_table.lock);

	return f;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
int
fdalloc(struct file *f)
{
	struct proc *curproc = myproc();

	for (int fd = 0; fd < OPEN_MAX; fd++) {
		if (curproc->ofile[fd] == NULL) {
			curproc->ofile[fd] = f;
			return fd;
		}
	}
	return -EMFILE;
}

// Allocate a file descriptor for the given file.
// Use the file descriptor specified in fd.
int
fdalloc2(struct file *f, int fd)
{
	struct proc *curproc = myproc();

	if (curproc->ofile[fd] != NULL) {
		PROPOGATE_ERR(vfs_close(curproc->ofile[fd]));
	}

	curproc->ofile[fd] = f;
	return fd;
}

// Read a symbolic link and puts the data in buf.
ssize_t
filereadlinkat(int dirfd, const char *restrict pathname, char *buf,
               size_t bufsiz)
{
	struct inode *ip = namei_with_fd(dirfd, pathname);
	if (ip == NULL) {
		return -ENOENT;
	}

	nlink_t ref_count = SYMLOOP_MAX;
	inode_lock(ip);

	if (!S_ISLNK(ip->mode)) {
		inode_unlockput(ip);
		return -EINVAL;
	}

	ssize_t nbytes = 0;

	while (S_ISLNK(ip->mode)) {
		// We can only resolve so many symlinks.
		// Error with ELOOP if we see too many.
		ref_count--;
		if (ref_count == 0) {
			inode_unlockput(ip);
			return -ELOOP;
		}

		nbytes = inode_read(ip, buf, 0, bufsiz);

		if (nbytes < 0) {
			inode_unlockput(ip);
			return nbytes;
		}

		inode_unlock(ip);

		if ((ip = namei(buf)) == NULL) {
			inode_put(ip);
			return -ENOENT;
		}
		inode_lock(ip);
	}
	inode_unlockput(ip);
	return nbytes;
}

// Holds lock on ip when released, unless it is NULL.
// This function essentially does three things if the file is not present:
// - allocate the inode [inode_alloc]
// - give it some default stats (more if a directory)
// - attach the file to a directory [dirlink]
struct inode *
vfs_locked_inode_create(int dirfd, char *path, mode_t mode, dev_t dev)
{
	struct inode *ip, *dp;
	char name[DIRSIZ];

	// POSIX says that creat(2) makes an IFREG if IFMT is 0.
	if ((mode & S_IFMT) == 0) {
		mode |= S_IFREG;
	}

	// get inode of path, and put the name in name.
	if ((dp = nameiparent_with_fd(dirfd, path, name)) == NULL) {
		return NULL;
	}
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

	if ((ip = inode_alloc(dp->dev, mode)) == NULL) {
		panic("create: inode_alloc");
	}

	inode_lock(ip);
	ip->major = major(dev);
	ip->minor = minor(dev);
	ip->nlink = 1;
	ip->mode = mode;
	ip->gid = DEFAULT_GID;
	ip->uid = DEFAULT_UID;
	ip->fattrs = 0;
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
		if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0) {
			panic("create dots");
		}
	}

	// Actually create the file entry.
	if (dirlink(dp, name, ip->inum) < 0) {
		panic("create: dirlink");
	}

	inode_unlockput(dp);

	return ip;
}

// Pass in a path name, and get out an inode pointing to
// a file after all dereferencing of symlinks, or NULL if
// the name doesn't exist or if filereadlink() has an error.
struct inode *
resolve_name(const char *path)
{
	return resolve_nameat(AT_FDCWD, path);
}

struct inode *
resolve_nameat(int dirfd, const char *path)
{
	struct inode *ip = namei_with_fd(dirfd, path);
	if (ip == NULL) {
		return NULL;
	}

	inode_lock(ip);
	bool is_link = S_ISLNK(ip->mode);
	inode_unlock(ip);

	if (is_link) {
		char buf[PATH_MAX] = {};
		ssize_t ret = filereadlinkat(dirfd, path, buf, sizeof(buf));
		if (ret < 0) {
			return NULL;
		}
		inode_put(ip);
		ip = namei_with_fd(dirfd, buf);
		return ip;
	} else {
		return ip;
	}
}

int
vfs_openat(int dirfd, char *path, int flags, mode_t mode)
{
	int fd;
	struct inode *ip;

	if (path == NULL) {
		return -EFAULT;
	}

	begin_op();

	if ((flags & O_CREATE) == O_CREATE) {
		// try to create a file and it exists.
		if ((ip = (((flags & O_NOFOLLOW) ? namei_with_fd :
		                                   resolve_nameat)(dirfd, path))) != NULL) {
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
		ip = vfs_locked_inode_create(dirfd, path, mode, 0);
		if (ip == NULL) {
			end_op();
			return -EIO;
		}
	} else {
		if ((ip = (((flags & O_NOFOLLOW) ? namei_with_fd :
		                                   resolve_nameat)(dirfd, path))) == NULL) {
			end_op();
			return -ENOENT;
		}
		inode_lock(ip);
		ip->fattrs = flags;

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
	// In either case (create or open), we need
	// to open the read and write ends of a FIFO or pipe.
	if (S_ISFIFO(ip->mode) && (flags & O_NONBLOCK) != O_NONBLOCK) {
		// TODO handle the case where ip->rf == NULL && ip->wf != NULL
		// and vice versa.
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

			ip->rf->pipe->writeopen = 0;
			ip->rf->pipe->readopen = 0;

			ip->wf->pipe->writeopen = 0;
			ip->wf->pipe->readopen = 0;
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

	struct file *f;
	// By this line, both branches above are holding a lock to ip.
	// That is why it is released down here.
get_fd:

	if ((f = filealloc()) == NULL || (fd = fdalloc(f)) < 0) {
		// Fileclose returns int but
		// we ignore it because we error out regardless.
		if (f) {
			(void)vfs_close(f);
		}
		inode_unlockput(ip);
		end_op();
		return -EMFILE;
	}
	// Don't put because we keep the reference in f->ip.
	inode_unlock(ip);
	end_op();

	// If this is a pipe, pipeopen() updates this value later.
	f->type = FD_INODE;
	f->ip = ip;
	f->off = (flags & O_APPEND) ? f->ip->size : 0;
	f->readable = ((flags & O_ACCMODE) == O_RDONLY) ||
	              ((flags & O_ACCMODE) == O_RDWR);
	f->writable = ((flags & O_ACCMODE) == O_WRONLY) ||
	              ((flags & O_ACCMODE) == O_RDWR);
	return fd;
}
// Close file f.  (Decrement ref count, close when reaches 0.)
int
vfs_close(struct file *f)
{
	acquire(&file_table.lock);
	if (__unlikely(f->ref < 1)) {
		panic("fileclose: file already closed");
	}
	// Is the file open somewhere else?
	// If so, just leave with success.
	if (--f->ref > 0) {
		release(&file_table.lock);
		return 0;
	}
	if (S_ISCHR(f->ip->mode)) {
		if (f->ip->major < 0 || f->ip->major >= NDEV ||
		    devsw[f->ip->major].close == NULL) {
			release(&file_table.lock);
			return -ENODEV;
		}
		// Run device-specific closing code, if any.
		devsw[f->ip->major].close(f->ip->minor);
	}
	struct file ff = *f;

	f->ref = 0;
	f->type = FD_NONE;
	f->flags = 0;
	release(&file_table.lock);

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
vfs_stat(struct file *f, struct stat *st)
{
	if (f->type == FD_INODE || f->type == FD_FIFO || f->type == FD_PIPE) {
		inode_lock(f->ip);
		inode_stat(f->ip, st);
		inode_unlock(f->ip);
		return 0;
	}
	return -ENOENT;
}

// Read from file f.
ssize_t
vfs_read(struct file *f, char *addr, size_t n)
{
	if (f->readable == 0) {
		return -EINVAL;
	}
	if (f->type == FD_PIPE || f->type == FD_FIFO) {
		return piperead(f->pipe, addr, n);
	} else if (f->type == FD_INODE) {
		inode_lock(f->ip);
		ssize_t r;
		if ((r = inode_read(f->ip, addr, f->off, n)) > 0) {
			// We have read this many bytes, so
			// increase the offset.
			f->off += r;
		}
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
vfs_write(struct file *f, char *addr, size_t n)
{
	if (f->writable == 0) {
		return -EROFS;
	}
	if (f->type == FD_PIPE || f->type == FD_FIFO) {
		return pipewrite(f->pipe, addr, n);
	}
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
			if (n1 > max) {
				n1 = max;
			}

			// begin_op()/end_op() is here because
			// we only need to setup the log
			// in the event of a write.
			begin_op();
			inode_lock(f->ip);
			ssize_t r;
			if ((r = inode_write(f->ip, addr + i, f->off, n1))) {
				f->off += r;
			}
			inode_unlock(f->ip);
			end_op();

			if (r < 0) {
				break;
			}

			if (r != n1) {
				panic("short filewrite: r=%ld, n1=%lu\n", r, n1);
			}
			i += r;
		}
		// Short writes are acceptable and may happen
		// for various reasons according to the standard.
		return (ssize_t)i;
	}
	panic("filewrite");
}

static int
name_of_inode(struct inode *ip, struct inode *parent, char buf[static DIRSIZ],
              size_t n)
{
	struct dirent de;
	for (off_t off = 0; off < parent->size; off += sizeof(de)) {
		PROPOGATE_ERR(inode_read(parent, (char *)&de, off, sizeof(de)));
		if (de.d_ino == ip->inum) {
			strncpy(buf, de.d_name, n - 1);
			return 0;
		}
	}
	return -1;
}

// Write n bytes to buf, including the null terminator.
// Returns the modified buf, or NULL.
char *
inode_to_path(char *buf, size_t n, struct inode *ip)
{
	struct inode *parent;
	char node_name[DIRSIZ] = {};
	bool isroot, isdir;
	{
		struct inode *tmp_ip = namei("/");
		kernel_assert(tmp_ip != NULL);
		// Inode number does not need to be protected by a lock.
		isroot = ip->inum == tmp_ip->inum;
		inode_put(tmp_ip);
	}
	inode_lock(ip);
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
		if (parent == NULL) {
			return NULL;
		}
		inode_lock(parent);
		if (name_of_inode(ip, parent, node_name, n) < 0) {
			inode_unlockput(parent);
			return NULL;
		}
		inode_unlock(parent);
		// 's' has the name lifetime as buf.
		char *s = inode_to_path(buf, n, parent);
		if (s == NULL) {
			return NULL;
		}
		inode_put(parent);
		if (strcmp(s, "/") != 0) {
			strncat(s, "/", 2);
		}
		// Returning s, with the same lifetime as buf.
		return strncat(s, node_name, DIRSIZ - strlen(node_name));
	} else {
		return NULL;
	}
}
