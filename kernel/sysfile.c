//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "lib/compiler_attributes.h"
#include "fb.h"
#include "fcntl_constants.h"
#include "memlayout.h"
#include "kernel_assert.h"
#include "mmu.h"
#include "pci.h"
#include "termios.h"
#include "vga.h"
#include <defs.h>
#include <stdint.h>
#include <stat.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <dirent.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include "param.h"
#include "types.h"
#include "proc.h"
#include "fs.h"
#include "file.h"
#include "console.h"
#include "log.h"
#include "syscall.h"
#include "pipe.h"
#include "exec.h"
#include "ioctl.h"
#include "kalloc.h"
#include "mman.h"
#include "pipe.h"
#include "lib/ring_buffer.h"
#include <string.h>
#include "drivers/lapic.h"
#include "vm.h"

static struct inode *
link_dereference(struct inode *ip, char *buff);
// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
	int fd;
	struct file *f;

	PROPOGATE_ERR(argint(n, &fd));
	if (fd < 0 || (f = myproc()->ofile[fd]) == NULL)
		return -EBADF;
	if (fd >= NOFILE)
		return -ENFILE;
	if (pfd)
		*pfd = fd;
	if (pf)
		*pf = f;
	return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
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

size_t
sys_dup(void)
{
	struct file *f;
	int fd;

	PROPOGATE_ERR(argfd(0, NULL, &f));
	PROPOGATE_ERR(fd = fdalloc(f));
	filedup(f);
	return fd;
}

size_t
sys_read(void)
{
	struct file *f;
	uint64_t n;
	char *p;

	// Do not rearrange, because then 'n' will be undefined.
	// argptr() uses the value of n here, and it is only
	// initialized after arguintptr_t() is run.
	PROPOGATE_ERR(argfd(0, NULL, &f));
	PROPOGATE_ERR(arguintptr_t(2, &n));
	PROPOGATE_ERR(argptr(1, &p, n));
	return fileread(f, p, n);
}

size_t
sys_write(void)
{
	struct file *f;
	uint64_t n;
	char *p;

	// Do not rearrange, because then 'n' will be undefined.
	// argptr() uses the value of n here, and it is only
	// initialized after arguintptr_t() is run.
	PROPOGATE_ERR(argfd(0, NULL, &f));
	PROPOGATE_ERR(arguintptr_t(2, &n));
	PROPOGATE_ERR(argptr(1, &p, n));
	return filewrite(f, p, n);
}

size_t
sys_writev(void)
{
	struct iovec *iovecs;
	int iovcnt;
	int fd;
	struct file *file;
	ssize_t accumulated_bytes = 0;
	PROPOGATE_ERR(argfd(0, &fd, &file));
	PROPOGATE_ERR(argint(2, &iovcnt));
	PROPOGATE_ERR(argptr(1, (void *)&iovecs, sizeof(*iovecs) * iovcnt));

	for (int i = 0; i < iovcnt; i++) {
		ssize_t ret = filewrite(file, iovecs->iov_base, iovecs->iov_len);
		if (ret < 0)
			return ret;
		accumulated_bytes += ret;
	}
	return accumulated_bytes;
}
size_t
sys_close(void)
{
	int fd;
	struct file *f;
	PROPOGATE_ERR(argfd(0, &fd, &f));

	myproc()->ofile[fd] = NULL;
	fileclose(f);
	return 0;
}

size_t
sys_fstat(void)
{
	struct file *f;
	struct stat *st;
	PROPOGATE_ERR(argfd(0, NULL, &f));
	PROPOGATE_ERR(argptr(1, (void *)&st, sizeof(*st)));

	if (st == NULL)
		return -EFAULT;
	return filestat(f, st);
}

size_t
sys_stat(void)
{
	struct stat *st;
	char *path;
	struct inode *ip = NULL;
	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(argptr(1, (void *)&st, sizeof(*st)));

	// Find the inode from the name.
	if ((ip = namei(path)) == NULL) {
		return -ENOENT;
	}
	if (st == NULL)
		return -EFAULT;

	// Everything that can use stat is an inode.
	inode_lock(ip);
	inode_stat(ip, st);
	inode_unlock(ip);
	return 0;
}

// Create the path new as a link to the same inode as old.
size_t
sys_link(void)
{
	char name[DIRSIZ], *new, *old;
	int retflag = EINVAL;
	struct inode *dp, *ip;

	PROPOGATE_ERR(argstr(0, &old));
	PROPOGATE_ERR(argstr(1, &new));

	begin_op();
	if ((ip = namei(old)) == NULL) {
		end_op();
		return -ENOENT;
	}

	inode_lock(ip);
	if (S_ISDIR(ip->mode)) {
		inode_unlockput(ip);
		end_op();
		return -EISDIR;
	}

	ip->nlink++;
	inode_update(ip);
	inode_unlock(ip);

	if ((dp = nameiparent(new, name)) == NULL) {
		retflag = ENOENT;
		goto bad;
	}
	inode_lock(dp);
	if (dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0) {
		inode_unlockput(dp);
		retflag = EXDEV; // probably incorrect
		goto bad;
	}
	inode_unlockput(dp);
	inode_put(ip);

	end_op();

	return 0;

bad:
	inode_lock(ip);
	ip->nlink--;
	inode_update(ip);
	inode_unlockput(ip);
	end_op();
	return -retflag;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
	struct dirent de;

	for (uint64_t off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
		if (inode_read(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
			panic("isdirempty: inode_read");
		if (de.d_ino != 0)
			return 0;
	}
	return 1;
}

size_t
sys_unlink(void)
{
	struct inode *ip, *dp;
	struct dirent de;
	char name[DIRSIZ], *path;
	uint64_t off;
	int error = EINVAL;

	PROPOGATE_ERR(argstr(0, &path));

	begin_op();
	if ((dp = nameiparent(path, name)) == NULL) {
		end_op();
		return -ENOENT;
	}

	inode_lock(dp);

	// Cannot unlink "." or "..".
	if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
		goto bad;

	if ((ip = dirlookup(dp, name, &off)) == NULL) {
		error = ENOENT;
		goto bad;
	}
	kernel_assert(ip != dp);

	inode_lock(ip);

	if (ip->nlink < 1)
		panic("unlink: nlink < 1");
	if (S_ISDIR(ip->mode) && !isdirempty(ip)) {
		inode_unlockput(ip);
		error = ENOTEMPTY;
		goto bad;
	}

	memset(&de, 0, sizeof(de));
	if (inode_write(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
		panic("unlink: inode_write");
	if (S_ISDIR(ip->mode)) {
		dp->nlink--;
		inode_update(dp);
	}
	inode_unlockput(dp);

	ip->nlink--;
	inode_update(ip);
	inode_unlockput(ip);

	end_op();

	return 0;

bad:
	inode_unlockput(dp);
	end_op();
	return -error;
}

// Holds lock on ip when released.
static struct inode *
create(char *path, mode_t mode, short major, short minor)
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
		// create() holds a lock on this inode pointer,
		// but only if it succeeds.
		ip = create(path, mode, 0, 0);
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
		if (f)
			fileclose(f);
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

size_t
sys_open(void)
{
	char *path;
	int flags;
	mode_t mode;

	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(argint(1, &flags));
	// Always pulled in because the libc wrapper
	// sets it to zero if the flags that require it are not set.
	PROPOGATE_ERR(argmode_t(2, &mode));
	if (!(((flags & O_CREAT) == O_CREAT)) && !(((flags & O_TMPFILE) == O_TMPFILE))) {
		mode = 0777; // mode is ignored.
	}

	// Myproc is NULL? Uh, ask it to try again later...
	if (myproc() == NULL)
		return -EAGAIN;

	return fileopen(path, flags, mode & ~(myproc()->umask));
}

size_t
sys_mkdir(void)
{
	char *path;
	struct inode *ip;
	mode_t mode;

	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(argmode_t(1, &mode));

	if (myproc() == NULL)
		return -EAGAIN;
	begin_op();
	if ((ip = create(path, (S_IFDIR | mode) & ~myproc()->umask, 0, 0)) == NULL) {
		end_op();
		return -ENOENT;
	}
	inode_unlockput(ip);
	end_op();
	return 0;
}

size_t
sys_mknod(void)
{
	struct inode *ip;
	char *path;
	int major, minor;
	mode_t mode;
	dev_t dev;

	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(argmode_t(1, &mode));
	PROPOGATE_ERR(argdev_t(2, &dev));

	begin_op();
	if ((ip = create(path, mode, major(dev), minor(dev))) == 0) {
		end_op();
		return -ENOENT;
	}
	inode_unlockput(ip);
	end_op();
	return 0;
}

size_t
sys_chdir(void)
{
	char *path;
	struct inode *ip;
	struct proc *curproc = myproc();

	PROPOGATE_ERR(argstr(0, &path));

	begin_op();
	if ((ip = namei(path)) == 0) {
		end_op();
		return -EINVAL;
	}
	inode_lock(ip);
	if (S_ISLNK(ip->mode)) {
		if ((ip = link_dereference(ip, path)) == 0) {
			inode_unlockput(ip);
			end_op();
			// POSIX says to return this if there is a "dangling symbolic link".
			return -ENOENT;
		}
	}
	if (!S_ISDIR(ip->mode)) {
		inode_unlockput(ip);
		end_op();
		return -ENOTDIR;
	}
	inode_unlock(ip);
	inode_put(curproc->cwd);
	end_op();
	curproc->cwd = ip;
	return 0;
}

size_t
sys_getcwd(void)
{
	char *buf;
	size_t size;
	PROPOGATE_ERR(argstr(0, &buf));
	PROPOGATE_ERR(argsize_t(1, &size));

	// Translate cwd from inode into path.
	char *ret = inode_to_path(buf, size, myproc()->cwd);
	// If we are negative, propogate the errno.
	if (ret == NULL)
		return -EINVAL;
	return 0;
}

size_t
sys_execve(void)
{
	char *path, *argv[MAXARG], *envp[MAXENV];
	uintptr_t uargv, uarg;
	uintptr_t uenvp, uenv;

	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(arguintptr_t(1, &uargv));
	PROPOGATE_ERR(arguintptr_t(2, &uenvp));

	memset(argv, 0, sizeof(argv));
	memset(envp, 0, sizeof(envp));
	for (size_t i = 0;; i++) {
		if (i >= NELEM(argv))
			return -EINVAL;
		if (fetchuintptr_t(uargv + sizeof(uintptr_t) * i, &uarg) < 0)
			return -EINVAL;
		if (uarg == 0) {
			argv[i] = NULL;
			break;
		}
		if (fetchstr(uarg, &argv[i]) < 0)
			return -EINVAL;
	}
	for (size_t i = 0;; i++) {
		if (i >= NELEM(envp))
			return -EINVAL;
		if (fetchuintptr_t(uenvp + sizeof(uintptr_t) * i, &uenv) < 0)
			return -EINVAL;
		if (uenv == 0) {
			envp[i] = NULL;
			break;
		}
		if (fetchstr(uenv, &envp[i]) < 0)
			return -EINVAL;
	}
	return execve(path, argv, envp);
}

size_t
sys_pipe(void)
{
	int fd[2];
	struct file *rf, *wf;
	int fd0, fd1;

	PROPOGATE_ERR(argptr(0, (void *)&fd, sizeof(fd)));

	PROPOGATE_ERR(pipealloc(&rf, &wf));
	fd0 = -1;
	if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
		if (fd0 >= 0)
			myproc()->ofile[fd0] = NULL;
		fileclose(rf);
		fileclose(wf);
		return -EBADF;
	}
	fd[0] = fd0;
	fd[1] = fd1;
	return 0;
}

size_t
sys_chmod(void)
{
	char *path;
	mode_t mode;
	struct inode *ip;
	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(argmode_t(1, &mode));

	begin_op();
	if ((ip = namei(path)) == NULL) {
		end_op();
		return -EINVAL;
	}
	inode_lock(ip);
	// capture the file type and change the permissions
	ip->mode = (ip->mode & S_IFMT) | mode;
	inode_unlock(ip);
	end_op();
	return 0;
}

size_t
sys_fchmod(void)
{
	struct file *file;
	mode_t mode;
	begin_op();
	PROPOGATE_ERR_WITH(argfd(0, NULL, &file), {
		end_op();
	});
	PROPOGATE_ERR_WITH(argmode_t(1, &mode), {
		end_op();
	});
	inode_lock(file->ip);
	file->ip->mode = (file->ip->mode & S_IFMT) | mode;
	inode_unlock(file->ip);
	end_op();
	return 0;
}

// target, linkpath
size_t
sys_symlink(void)
{
	char *target, *linkpath;
	char dir[DIRSIZ];
	uint64_t poff;
	struct inode *eexist, *ip;
	PROPOGATE_ERR(argstr(0, &target));
	PROPOGATE_ERR(argstr(1, &linkpath));

	begin_op();
	if ((eexist = namei(linkpath)) != NULL) {
		end_op();
		return -EEXIST;
	}
	if ((eexist = nameiparent(linkpath, dir)) == NULL) {
		end_op();
		return -ENOENT;
	}

	// Dirlookup's first arg needs a lock.
	inode_lock(eexist);

	if ((ip = dirlookup(eexist, dir, &poff)) != NULL) {
		inode_unlockput(eexist);
		end_op();
		return -EEXIST;
	}
	inode_unlock(eexist);

	if ((ip = create(linkpath, S_IFLNK | S_IAUSR, 0, 0)) == NULL) {
		end_op();
		return -ENOSPC;
	}
	if (inode_write(ip, target, 0, strlen(target) + 1) != strlen(target) + 1)
		panic("symlink inode_write");

	inode_unlockput(ip);
	end_op();

	return 0;
}

size_t
sys_readlink(void)
{
	char *target, *ubuf;
	size_t bufsize = 0;
	PROPOGATE_ERR(argstr(0, &target));
	PROPOGATE_ERR(argstr(1, &ubuf));
	PROPOGATE_ERR(argsize_t(2, &bufsize));

	struct inode *ip;
	begin_op();
	if ((ip = namei(target)) == NULL) {
		return -ENOENT;
	}

	inode_lock(ip);

	if (!S_ISLNK(ip->mode)) {
		inode_unlock(ip);
		end_op();
		return -EINVAL;
	}

	if (ip->size > bufsize) {
		inode_unlock(ip);
		end_op();
		return -EINVAL;
	}

	if (inode_read(ip, ubuf, 0, bufsize) < 0)
		panic("readlink inode_read");

	if (copyout(myproc()->pgdir, (uintptr_t)ubuf, ubuf, bufsize) < 0)
		panic("readlink copyout");

	inode_unlock(ip);
	end_op();
	return 0;
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

size_t
sys_lseek(void)
{
	int fd;
	off_t offset;
	int whence;
	struct file *file;

	PROPOGATE_ERR(argfd(0, &fd, &file));
	PROPOGATE_ERR(argoff_t(1, &offset));
	PROPOGATE_ERR(argint(2, &whence));

	if (S_ISFIFO(file->ip->mode) || S_ISSOCK(file->ip->mode))
		return -ESPIPE;

	return fileseek(file, offset, whence);
}

size_t
sys_ioctl(void)
{
	int fd;
	struct file *file;
	unsigned long request;
	uintptr_t uptr;
	PROPOGATE_ERR(argfd(0, &fd, &file));
	PROPOGATE_ERR(argunsigned_long(1, &request));

	// The file needs to be a char device.
	if (!S_ISCHR(file->ip->mode))
		return -ENOTTY;

	switch (request) {
	case PCIIOCGETCONF: {
		struct pci_conf *pci_conf_p;

		PROPOGATE_ERR(argptr(2, (char **)&pci_conf_p, sizeof(struct pci_conf *)));

		if (pci_conf_p == NULL)
			return -EFAULT;

		// INVARIANT: pci_init must happen before pci_get_conf().
		struct FatPointerArray_pci_conf pci_conf = pci_get_conf();

		memcpy(pci_conf_p, pci_conf.ptr,
					 pci_conf.len * sizeof(struct pci_conf));
		return 0;
		break;
	}
	case FBIOCGET_VSCREENINFO: {
		if (file->ip->major != FB) {
				return -EINVAL;
		}
		struct fb_var_screeninfo *scr_info;
		PROPOGATE_ERR(argptr(2, (char **)&scr_info,
											 sizeof(struct fb_var_screeninfo *)));

		if (scr_info == NULL)
			return -EFAULT;

		struct fb_var_screeninfo info = { WIDTH, HEIGHT, BPP_DEPTH };
		memcpy(scr_info, &info, sizeof(struct fb_var_screeninfo));
		return 0;
		break;
	}
	case TIOCGETAW:
	case TIOCGETAF:
	case TIOCGETA: {
		if (file->ip->major != TTY) {
			return -EINVAL;
		}
		struct termios *termios;
		PROPOGATE_ERR(argptr(2, (char **)&termios, sizeof(struct termios *)));

		if (termios == NULL)
			return -EFAULT;

		struct termios *ret = get_term_settings(file->ip->minor);
		// Copy here because userspace cannot use kernel pointers.
		memcpy(termios, ret, sizeof(struct termios));
		return 0;
		break;
	}
	case TIOCSETAW:
	case TIOCSETAF:
	case TIOCSETA: {
		if (file->ip->major != TTY) {
			return -EINVAL;
		}
		struct termios *termios;
		PROPOGATE_ERR(argptr(2, (char **)&termios, sizeof(struct termios *)));

		if (termios == NULL)
			return -EFAULT;

		set_term_settings(file->ip->minor, termios);
		// Adjust whether chracters print.
		echo_out = (termios->c_lflag & ECHO) == ECHO;

		return 0;
		break;
	}
	default: {
		return -EINVAL;
	}
	}
}

size_t
sys_fcntl(void)
{
	struct file *file;
	int op;
	PROPOGATE_ERR(argfd(0, NULL, &file));
	PROPOGATE_ERR(argint(1, &op));


	switch (op) {
	case F_DUPFD: {
		int arg;
		int fd;
		PROPOGATE_ERR(argint(2, &arg));
		PROPOGATE_ERR(fd = fdalloc(file));
		if (filedup(file) == NULL)
			return -EBADF;
		return fd;
	}
	case F_DUPFD_CLOEXEC: {
		int arg;
		int fd;
		struct file *duped_file;
		PROPOGATE_ERR(argint(2, &arg));
		PROPOGATE_ERR(fd = fdalloc(file));
		if ((duped_file = filedup(file)) == NULL)
			return -EBADF;
		duped_file->flags = FD_CLOEXEC;
		return fd;
	}
	case F_GETFL: {
		inode_lock(file->ip);
		int flags = file->ip->flags;
		inode_unlock(file->ip);
		return flags;
	}
	case F_SETFL: {
		int arg;
		PROPOGATE_ERR(argint(2, &arg));
		inode_lock(file->ip);
		file->ip->flags = arg;
		inode_unlock(file->ip);
		return 0;
	}
	case F_GETFD: {
		return file->flags;
	}
	case F_SETFD: {
		int arg;
		PROPOGATE_ERR(argint(2, &arg));
		file->flags = arg;
		return 0;
	}
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
	case F_GETLKW:
	case F_GETOWN:
	case F_SETOWN:
		return -ENOSYS;
	default: {
		return -EINVAL;
	}
	}
	// Not reached.
}

static int
mmap_prot_to_perm(int prot)
{
	int ret = PTE_U;
	if ((prot & PROT_READ) == PROT_READ) {
		// No PTE flag for this
	}
	if ((prot & PROT_WRITE) == PROT_WRITE) {
		ret |= PTE_W;
	}
	return ret;
}
size_t
sys_mmap(void)
{
	void *addr;
	size_t length;
	int prot, flags, fd;
	struct file *file;
	off_t offset;
	PROPOGATE_ERR(argptr(0, (char **)&addr, sizeof(void *)));
	PROPOGATE_ERR(argsize_t(1, &length));
	PROPOGATE_ERR(argint(2, &prot));
	PROPOGATE_ERR(argint(3, &flags));
	PROPOGATE_ERR(argfd(4, &fd, &file));
	PROPOGATE_ERR(argoff_t(5, &offset));

	if (length == 0)
		return -EINVAL;
	// We don't support MAP_PRIVATE or MAP_SHARED_VALIDATE for now.
	if ((flags & MAP_SHARED) != MAP_SHARED)
		return -EINVAL;

	// "The file has been locked, or too much memory has been locked"
	if (file->ip->lock.locked)
		return -EAGAIN;
	if (length % PGSIZE != 0 || (uintptr_t)addr % PGSIZE != 0)
		return -EINVAL;
	int perm = mmap_prot_to_perm(prot);
	struct mmap_info info;
	if (S_ISCHR(file->ip->mode)) {
		if (file->ip->major < 0 || file->ip->major >= NDEV ||
				!devsw[file->ip->major].mmap)
			return -ENODEV;
		info = devsw[file->ip->major].mmap(file->ip->minor, length, (uintptr_t)addr,
																			 perm);
		info.file = file;
	} else {
		info = (struct mmap_info){ length, (uintptr_t)addr, 0 /* virtual address */,
															 NULL, perm };
	}
	struct proc *proc = myproc();
	if (proc->mmap_count > NMMAP)
		return -ENOMEM;
	// Place it anywhere.
	if (addr == NULL) {
		if (info.virt_addr == 0)
			info.virt_addr = PGROUNDUP(proc->effective_largest_sz);
		if (mappages(myproc()->pgdir, (void *)info.virt_addr, info.length,
								 info.addr, info.perm) < 0) {
			return -ENOMEM;
		}
		myproc()->effective_largest_sz += info.length;
		proc->mmap_info[proc->mmap_count++] = info;
		return (size_t)info.virt_addr;
	} else {
		if (mappages(proc->pgdir, addr, info.length, info.addr, info.perm) < 0) {
			void *ptr = kmalloc(info.length);
			if (ptr == NULL)
				return -ENOMEM;
			info.addr = V2P(ptr);
			if (mappages(proc->pgdir, addr, info.length, info.addr, info.perm) < 0) {
				kfree(ptr);
				return -ENOMEM;
			}
			proc->mmap_info[proc->mmap_count++] = info;
			return (size_t)ptr;
		}
		proc->mmap_info[proc->mmap_count++] = info;
		return (size_t)addr;
	}
}

size_t
sys_munmap(void)
{
	void *addr;
	size_t length;

	struct proc *proc = myproc();

	PROPOGATE_ERR(argptr(0, (char **)&addr, sizeof(void *)));
	PROPOGATE_ERR(argsize_t(1, &length));

	if (length == 0)
		return -EINVAL;
	int j = -1;
	for (int i = 0; i < NMMAP; i++) {
		if ((proc->mmap_info[i].virt_addr == (uintptr_t)addr) &&
				((proc->mmap_info[i].length == length) ||
				 (proc->mmap_info[i].file &&
					S_ISCHR(proc->mmap_info[i].file->ip->mode)))) {
			j = i;
		}
	}
	if (j == -1)
		return -EINVAL;
	for (int i = 0; i < PGROUNDUP(proc->mmap_info[j].length); i += PGSIZE) {
		unmap_user_page(proc->pgdir, (char *)proc->mmap_info[j].virt_addr);
	}
	if (addr != NULL && V2P(addr) < KERNBASE)
		kfree(addr);
	return 0;
}

/* Unimplemented */
size_t
sys_fsync(void)
{
	int fd;
	struct file *file;
	PROPOGATE_ERR(argfd(0, &fd, &file));

	if (fd == 0 || fd == 1 || fd == 2) {
		if (fd == 0)
			return -EINVAL;
		return 0;
	}
	return 0;
}

size_t
sys_umask(void)
{
	mode_t mask;
	// The default is S_IWGRP | S_IWOTH (022).
	// This function is said to never fail, so
	// we must return something useful.
	if (argint(0, &mask) < 0) {
		return S_IWGRP | S_IWOTH;
	}
	return myproc() ? myproc()->umask : (S_IWGRP | S_IWOTH);
}

size_t
sys_rename(void)
{
	char *oldpath_;
	char *newpath_;
	char dir[DIRSIZ];
	char newdir[DIRSIZ];
	struct inode *ip1, *ip2;
	struct dirent de;
	char newelem[DIRSIZ];

	PROPOGATE_ERR(argstr(0, &oldpath_));
	PROPOGATE_ERR(argstr(1, &newpath_));
	// argstr wants a mutable char *, but our arguments are const.
	// we cast them back here.
	const char *oldpath = oldpath_;
	const char *newpath = newpath_;

	begin_op();
	if ((ip1 = namei(oldpath)) == NULL) {
		end_op();
		return -ENOENT;
	}
	if ((ip2 = nameiparent(newpath, newelem)) == NULL) {
		end_op();
		return -ENOENT;
	}
	struct inode *dp = nameiparent(oldpath, dir);
	struct inode *new_dp = nameiparent(newpath, newdir);
	if (dp == NULL || new_dp == NULL) {
		end_op();
		return -ENOENT;
	}

	inode_lock(dp);
	// This starts at "2 * sizeof(de)" in order to skip "." and "..".
	for (off_t off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
		// By this point, we know that the dirent should exist, but this
		// error-out condition leans on the cautious side.
		if (inode_read(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) {
			end_op();
			return -ENOENT;
		}
		if (strncmp(de.d_name, dir, strlen(dir)) == 0) {
			uint16_t inode = de.d_ino;

			// Remove the old entry.
			memset(&de, '\0', sizeof(de));
			if (inode_write(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
				panic("rename: inode_write");

			// In the case of "mv /foo /bar", we use the same directory pointer (inode).
			// In that case, we do not want to try and lock the same inode twice.
			if (new_dp != dp)
				inode_lock(new_dp);
			// Add the file to the directory.
			// Reuse the inode number, but have a new name.
			if (dirlink(new_dp, newelem, inode) < 0) {
				if (new_dp != dp)
					inode_unlockput(new_dp);
				return -EEXIST;
			}
			if (new_dp != dp)
				inode_unlockput(new_dp);

			// Commit to disk.
			inode_unlockput(dp);
			end_op();
			return 0;
		}
	}

	inode_unlock(dp);
	end_op();
	return -ENOENT;
}

size_t
sys_access(void)
{
	char *path;
	// Not to be confused with the
	// mode_t mode.
	int mode;
	struct inode *ip;
	// Initialized down below.
	bool perms_wanted_ok;
	bool read_ok = false;
	bool write_ok = false;
	bool exec_ok = false;
	bool correct_uid;
	bool correct_gid;
	PROPOGATE_ERR(argstr(0, &path));
	PROPOGATE_ERR(argint(1, &mode));
	begin_op();
	// File does not exist,
	if ((ip = namei(path)) == NULL) {
		end_op();
		return -ENOENT;
	}
	// All we wanted to do is check whether the
	// file exists, and it does.
	if (mode == F_OK) {
		end_op();
		return 0;
	}
	// This small bit is the only part that
	// requires a lock since the booleans are
	// evaluated here (the only use of ip).
	inode_lock(ip);
	// UID 0 (root) is always the "correct UID".
	if (myproc()->cred.uid == 0) {
		correct_uid = true;
	} else {
		correct_uid = myproc()->cred.uid == ip->uid;
	}
	correct_gid = is_in_group(ip->gid, &myproc()->cred);

	inode_unlock(ip);
	end_op();

	// Check for read.
	if ((mode & R_OK) == R_OK) {
		// User
		if ((mode & S_IRUSR) == S_IRUSR) {
			read_ok |= correct_uid;
		}
		if ((mode & S_IRGRP) == S_IRGRP) {
			read_ok |= correct_gid;
		}
		if ((mode & S_IROTH) == S_IROTH) {
			read_ok |= true;
		}
	} else {
		read_ok = true;
	}

	if ((mode & W_OK) == W_OK) {
		// User
		if ((mode & S_IWUSR) == S_IWUSR) {
			write_ok |= correct_uid;
		}
		if ((mode & S_IWGRP) == S_IWGRP) {
			write_ok |= correct_gid;
		}
		if ((mode & S_IWOTH) == S_IWOTH) {
			write_ok |= true;
		}
	} else {
		write_ok = true;
	}

	if ((mode & X_OK) == X_OK) {
		// User
		if ((mode & S_IXUSR) == S_IXUSR) {
			exec_ok |= correct_uid;
		}
		if ((mode & S_IXGRP) == S_IXGRP) {
			exec_ok |= correct_gid;
		}
		if ((mode & S_IXOTH) == S_IXOTH) {
			exec_ok |= true;
		}
	} else {
		exec_ok = true;
	}


	perms_wanted_ok = read_ok && write_ok && exec_ok;

	if (perms_wanted_ok) {
		return 0;
	} else {
		return -1;
	}
}
