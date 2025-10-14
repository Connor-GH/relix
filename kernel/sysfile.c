//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "console.h"
#include "exec.h"
#include "fb.h"
#include "file.h"
#include "fs.h"
#include "ioctl.h"
#include "kalloc.h"
#include "kernel_assert.h"
#include "log.h"
#include "macros.h"
#include "memlayout.h"
#include "mman.h"
#include "mmu.h"
#include "pci.h"
#include "pipe.h"
#include "proc.h"
#include "syscall.h"
#include "termios.h"
#include "trap.h"
#include "vga.h"
#include "vm.h"
#include <bits/access_constants.h>
#include <bits/fcntl_constants.h>
#include <bits/seek_constants.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
	int fd;
	struct file *f;

	PROPOGATE_ERR(argint(n, &fd));
	if (fd == AT_FDCWD) {
		if (pfd) {
			*pfd = fd;
		}
		return 0;
	}
	if (fd < 0) {
		return -EBADF;
	}
	if (fd >= OPEN_MAX) {
		return -ENFILE;
	}
	if ((f = myproc()->ofile[fd]) == NULL) {
		return -EBADF;
	}

	if (pfd != NULL) {
		*pfd = fd;
	}
	if (pf) {
		*pf = f;
	}
	return 0;
}

size_t
sys_dup3(void)
{
	struct file *f;
	int fd1;
	int fd2;
	int oflags;

	PROPOGATE_ERR(argfd(0, &fd1, &f));
	PROPOGATE_ERR(argint(1, &fd2));
	PROPOGATE_ERR(argint(2, &oflags));

	if (oflags & ~(O_CLOEXEC | O_CLOFORK)) {
		return -EINVAL;
	}

	if (fd2 < 0 && fd2 >= OPEN_MAX) {
		return -EBADF;
	}

	if (fd1 == fd2) {
		return fd2;
	}

	PROPOGATE_ERR(fd2 = fdalloc2(f, fd2));
	filedup(f, oflags);
	return fd2;
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
	return vfs_read(f, p, n);
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
	return vfs_write(f, p, n);
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
		ssize_t ret = vfs_write(file, iovecs->iov_base, iovecs->iov_len);
		if (ret < 0) {
			return ret;
		}
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
	return vfs_close(f);
}

size_t
sys_fstat(void)
{
	struct file *f;
	struct stat *st;
	PROPOGATE_ERR(argfd(0, NULL, &f));
	PROPOGATE_ERR(argptr(1, (void *)&st, sizeof(*st)));

	if (st == NULL) {
		return -EFAULT;
	}
	return vfs_stat(f, st);
}

size_t
sys_fstatat(void)
{
	struct stat *st;
	char *path;
	int dirfd;
	int flags;
	struct inode *ip = NULL;
	PROPOGATE_ERR(argfd(0, &dirfd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argptr(2, (char **)&st, sizeof(*st)));
	PROPOGATE_ERR(argint(3, &flags));

	// Find the inode from the name.
	if ((ip = ((flags & AT_SYMLINK_NOFOLLOW) ? namei_with_fd : resolve_nameat)(
				 dirfd, path)) == NULL) {
		return -ENOENT;
	}
	if (st == NULL) {
		return -EFAULT;
	}

	// Everything that can use stat is an inode.
	inode_lock(ip);
	inode_stat(ip, st);
	inode_unlockput(ip);
	return 0;
}

// Create the path new as a link to the same inode as old.
size_t
sys_linkat(void)
{
	char name[DIRSIZ], *new, *old;
	int retflag = EINVAL;
	struct inode *dp, *ip;

	int fd1;
	int fd2;
	int flags;

	PROPOGATE_ERR(argfd(0, &fd1, NULL));
	PROPOGATE_ERR(argstr(1, &old));
	PROPOGATE_ERR(argfd(2, &fd2, NULL));
	PROPOGATE_ERR(argstr(3, &new));
	PROPOGATE_ERR(argint(4, &flags));

	begin_op();
	if ((ip = ((flags & AT_SYMLINK_FOLLOW) ? resolve_nameat :
	                                         namei_with_fd)(fd1, old)) == NULL) {
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

	if ((dp = nameiparent_with_fd(fd2, new, name)) == NULL) {
		retflag = -ENOENT;
		goto bad;
	}
	inode_lock(dp);
	// TODO: When we get different filesystem support, we need to
	// check for differing filesystems here and return EXDEV.
	int ret;
	if ((ret = dirlink(dp, name, ip->inum)) < 0) {
		inode_unlockput(dp);
		retflag = ret; // probably incorrect
		goto bad;
	}
	if (dp != NULL) {
		inode_unlockput(dp);
	}
	inode_put(ip);

	end_op();

	return 0;

bad:
	inode_lock(ip);
	ip->nlink--;
	inode_update(ip);
	inode_unlockput(ip);
	end_op();
	return retflag;
}

// Is the directory dp empty except for "." and ".." ?
static bool
isdirempty(struct inode *dp)
{
	struct dirent de;

	for (off_t off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
		if (inode_read(dp, (char *)&de, off, sizeof(de)) < 0) {
			panic("isdirempty: inode_read");
		}
		if (de.d_ino != 0) {
			return false;
		}
	}
	return true;
}

size_t
sys_unlinkat(void)
{
	struct inode *ip, *dp;
	struct dirent de;
	char name[DIRSIZ], *path;
	uint64_t off;
	int fd;
	int flags;
	int error = EINVAL;

	PROPOGATE_ERR(argfd(0, &fd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argint(2, &flags));

	begin_op();
	if ((dp = nameiparent_with_fd(fd, path, name)) == NULL) {
		end_op();
		return -ENOENT;
	}

	inode_lock(dp);

	// Cannot unlink "." or "..".
	if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0) {
		goto bad;
	}

	if ((ip = dirlookup(dp, name, &off)) == NULL) {
		error = -ENOENT;
		goto bad;
	}
	kernel_assert(ip != dp);

	inode_lock(ip);

	// If this file has no links,
	// why are we trying to remove it?
	if (ip->nlink < 1) {
		inode_unlock(ip);
		end_op();
		return -ENOENT;
	}
	if (((S_ISDIR(ip->mode)) || (flags & AT_REMOVEDIR)) && !isdirempty(ip)) {
		inode_unlockput(ip);
		error = -ENOTEMPTY;
		goto bad;
	}

	// Write all zeroes to the place where the data was.
	memset(&de, 0, sizeof(de));
	PROPOGATE_ERR_WITH(inode_write(dp, (char *)&de, off, sizeof(de)), {
		inode_unlockput(ip);
		inode_unlockput(dp);
		end_op();
	});

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
	return error;
}

size_t
sys_openat(void)
{
	char *path;
	int flags;
	mode_t mode;
	int dirfd;

	PROPOGATE_ERR(argfd(0, &dirfd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argint(2, &flags));
	// Always pulled in because the libc wrapper
	// sets it to zero if the flags that require it are not set.
	PROPOGATE_ERR(argmode_t(3, &mode));
	if (!(((flags & O_CREAT) == O_CREAT)) &&
	    !(((flags & O_TMPFILE) == O_TMPFILE))) {
		mode = 0777; // mode is ignored.
	}

	// Myproc is NULL? Uh, ask it to try again later...
	if (myproc() == NULL) {
		return -EAGAIN;
	}

	return vfs_openat(dirfd, path, flags, mode & ~(myproc()->umask));
}

static int
stat_type_to_getdents_type(mode_t mode)
{
	switch (mode) {
	case S_IFBLK:
		return DT_BLK;
	case S_IFCHR:
		return DT_CHR;
	case S_IFDIR:
		return DT_DIR;
	case S_IFIFO:
		return DT_FIFO;
	case S_IFLNK:
		return DT_LNK;
	case S_IFSOCK:
		return DT_SOCK;
	case S_IFREG:
		return DT_REG;
	default:
		return DT_UNKNOWN;
	}
}

size_t
sys_getdents(void)
{
	struct file *file;
	void *buf;
	size_t nbyte;
	int flags;
	PROPOGATE_ERR(argfd(0, NULL, &file));
	PROPOGATE_ERR(argptr(1, (char **)&buf, sizeof(void *)));
	PROPOGATE_ERR(argsize_t(2, &nbyte));
	PROPOGATE_ERR(argint(3, &flags));

	if (flags & ~(DT_FORCE_TYPE)) {
		return -EINVAL;
	}

	size_t nread = 0;
	size_t dir_size;

	inode_lock(file->ip);
	dir_size = file->ip->size;
	inode_unlock(file->ip);

	// If we reach the end of the directory, exit.
	while (file->off < dir_size) {
		struct posix_dent *buf_loc = buf + nread;
		struct dirent de;
		PROPOGATE_ERR(vfs_read(file, (char *)&de, sizeof(struct dirent)));
		// This directory entry was deleted, so skip over it.
		if (de.d_ino == 0) {
			continue;
		}
		// Use offsetof to get the offset as sizeof will not be helpful here.
		// The true size of the struct is this calculation, before we round it
		// to the alignment.
		size_t expected_size =
			ROUND_UP(offsetof(struct posix_dent, d_name) + strlen(de.d_name) + 1,
		           alignof(struct posix_dent));

		// Also check for the case where the
		// expected size is over the size
		// of our buffer. In that case,
		// seek the file back where it was
		// and exit.
		if (nread + expected_size >= nbyte) {
			fileseek(file, -(off_t)sizeof(struct dirent), SEEK_CUR);
			break;
		}

		buf_loc->d_ino = de.d_ino;

		// Copy file name and truncate result if necessary.
		// We use strlen() instead of sizeof() because posix_dent
		// has a flexible array member for the file name.
		strncpy(buf_loc->d_name, de.d_name, strlen(de.d_name));
		buf_loc->d_name[strlen(de.d_name)] = '\0';

		// POSIX.1-2024 recommends that this flag is implemented, so we do.
		if (flags & DT_FORCE_TYPE) {
			struct inode *temp_ip = inode_get(file->ip->dev, de.d_ino);
			inode_lock(temp_ip);
			buf_loc->d_type = stat_type_to_getdents_type(temp_ip->mode & S_IFMT);
			inode_unlockput(temp_ip);
		} else {
			// When we get different filesystems, we might be able to get directory
			// entry types without searching it from the inode number.
			buf_loc->d_type = DT_UNKNOWN;
		}

		buf_loc->d_reclen = expected_size;

		nread += buf_loc->d_reclen;
	}

	return nread;
}

size_t
sys_mkdirat(void)
{
	char *path;
	struct inode *ip;
	mode_t mode;
	int fd;

	PROPOGATE_ERR(argfd(0, &fd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argmode_t(2, &mode));

	if (myproc() == NULL) {
		return -EAGAIN;
	}
	begin_op();
	if ((ip = vfs_locked_inode_create(
				 fd, path, (S_IFDIR | mode) & ~myproc()->umask, 0)) == NULL) {
		end_op();
		return -ENOENT;
	}
	inode_unlockput(ip);
	end_op();
	return 0;
}

size_t
sys_mknodat(void)
{
	struct inode *ip;
	char *path;
	mode_t mode;
	dev_t dev;
	int fd;

	PROPOGATE_ERR(argfd(0, &fd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argmode_t(2, &mode));
	PROPOGATE_ERR(argdev_t(3, &dev));

	begin_op();
	if ((ip = vfs_locked_inode_create(fd, path, mode, dev)) == NULL) {
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
	if ((ip = resolve_name(path)) == 0) {
		end_op();
		return -EINVAL;
	}
	inode_lock(ip);

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
	if (ret == NULL) {
		return -EINVAL;
	}
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
		if (i >= MAXARG) {
			return -E2BIG;
		}
		if (fetchuintptr_t(uargv + sizeof(uintptr_t) * i, &uarg) < 0) {
			return -EINVAL;
		}
		if (uarg == 0) {
			argv[i] = NULL;
			break;
		}
		if (fetchstr(uarg, &argv[i]) < 0) {
			return -EINVAL;
		}
	}
	for (size_t i = 0;; i++) {
		if (i >= MAXENV) {
			return -E2BIG;
		}
		if (fetchuintptr_t(uenvp + sizeof(uintptr_t) * i, &uenv) < 0) {
			return -EINVAL;
		}
		if (uenv == 0) {
			envp[i] = NULL;
			break;
		}
		if (fetchstr(uenv, &envp[i]) < 0) {
			return -EINVAL;
		}
	}
	return execve(path, argv, envp);
}

size_t
sys_pipe2(void)
{
	int *fd;
	struct file *rf, *wf;
	int fd0, fd1;
	int oflags;

	// Arrays don't decay like you'd expect them to
	// when going into argptr. You must use a raw
	// pointer type, even for arrays.
	PROPOGATE_ERR(argptr(0, (char **)&fd, 2 * sizeof(fd[0])));
	PROPOGATE_ERR(argint(1, &oflags));

	if (oflags & ~(O_CLOEXEC | O_CLOFORK | O_NONBLOCK)) {
		return -EINVAL;
	}

	PROPOGATE_ERR(pipealloc(&rf, &wf));
	fd0 = -1;
	if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
		if (fd0 >= 0) {
			myproc()->ofile[fd0] = NULL;
		}
		// Ignore the return value here so that
		// we can get a more accurate errno.
		(void)vfs_close(rf);
		(void)vfs_close(wf);
		return -EBADF;
	}
	rf->flags = oflags;
	wf->flags = oflags;
	fd[0] = fd0;
	fd[1] = fd1;
	return 0;
}

size_t
sys_fchmodat(void)
{
	char *path;
	mode_t mode;
	int fd;
	int flags;
	struct inode *ip;
	PROPOGATE_ERR(argfd(0, &fd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argmode_t(2, &mode));
	PROPOGATE_ERR(argint(3, &flags));

	begin_op();
	if ((ip = ((flags & AT_SYMLINK_NOFOLLOW) ? namei_with_fd : resolve_nameat)(
				 fd, path)) == NULL) {
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
	PROPOGATE_ERR_WITH(argfd(0, NULL, &file), { end_op(); });
	PROPOGATE_ERR_WITH(argmode_t(1, &mode), { end_op(); });
	if (file == NULL) {
		return -ENOENT;
	}
	inode_lock(file->ip);
	file->ip->mode = (file->ip->mode & S_IFMT) | mode;
	inode_unlock(file->ip);
	end_op();
	return 0;
}

// target, linkpath
size_t
sys_symlinkat(void)
{
	char *target, *linkpath;
	char dir[DIRSIZ];
	uint64_t poff;
	struct inode *eexist, *ip;
	int newdirfd;
	PROPOGATE_ERR(argstr(0, &target));
	PROPOGATE_ERR(argfd(1, &newdirfd, NULL));
	PROPOGATE_ERR(argstr(2, &linkpath));

	begin_op();
	if ((eexist = namei_with_fd(newdirfd, linkpath)) != NULL) {
		end_op();
		return -EEXIST;
	}
	if ((eexist = nameiparent_with_fd(newdirfd, linkpath, dir)) == NULL) {
		end_op();
		return -ENOENT;
	}

	// Dirlookup's first arg needs a lock.
	inode_lock(eexist);

	if ((ip = dirlookup(eexist, dir, &poff)) != NULL) {
		inode_unlockput(eexist);
		inode_put(ip);
		end_op();
		return -EEXIST;
	}
	inode_unlockput(eexist);

	if ((ip = vfs_locked_inode_create(newdirfd, linkpath, S_IFLNK | S_IAUSR,
	                                  0)) == NULL) {
		end_op();
		return -ENOSPC;
	}
	// Write the string, including the NUL terminator. We need the NUL as we use
	// it when dereferencing in resolve_name().
	// TODO: this causes an extra byte for the size of a symlink. We should be
	// handling this better.
	PROPOGATE_ERR_WITH(inode_write(ip, target, 0, strlen(target) + 1), {
		inode_unlockput(ip);
		end_op();
	})

	inode_unlockput(ip);
	end_op();

	return 0;
}

size_t
sys_readlinkat(void)
{
	char *target;
	char *ubuf;
	size_t bufsize = 0;
	int fd;
	PROPOGATE_ERR(argfd(0, &fd, NULL));
	PROPOGATE_ERR(argstr(1, &target));
	PROPOGATE_ERR(argstr(2, &ubuf));
	PROPOGATE_ERR(argsize_t(3, &bufsize));

	const char *restrict pathname = target;
	char *restrict buf = ubuf;

	begin_op();
	ssize_t ret = filereadlinkat(fd, pathname, buf, bufsize);
	end_op();
	return ret;
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
	if (file == NULL) {
		return -ENOENT;
	}

	if (S_ISFIFO(file->ip->mode) || S_ISSOCK(file->ip->mode)) {
		return -ESPIPE;
	}

	return fileseek(file, offset, whence);
}

size_t
sys_ioctl(void)
{
	int fd;
	struct file *file;
	unsigned long request;
	PROPOGATE_ERR(argfd(0, &fd, &file));
	PROPOGATE_ERR(argunsigned_long(1, &request));

	if (file == NULL) {
		return -ENOENT;
	}
	// The file needs to be a char device.
	if (!S_ISCHR(file->ip->mode)) {
		return -ENOTTY;
	}

	switch (request) {
	case PCIIOCGETCONF: {
		struct pci_conf *pci_conf_p;

		PROPOGATE_ERR(argptr(2, (char **)&pci_conf_p, sizeof(struct pci_conf *)));

		if (pci_conf_p == NULL) {
			return -EFAULT;
		}

		// INVARIANT: pci_init must happen before pci_get_conf().
		struct FatPointerArray_pci_conf pci_conf = pci_get_conf();

		memcpy(pci_conf_p, pci_conf.ptr, pci_conf.len * sizeof(struct pci_conf));
		return 0;
		break;
	}
	case FBIOCGET_VSCREENINFO: {
		if (file->ip->major != DEV_FB) {
			return -EINVAL;
		}
		struct fb_var_screeninfo *scr_info;
		PROPOGATE_ERR(
			argptr(2, (char **)&scr_info, sizeof(struct fb_var_screeninfo *)));

		if (scr_info == NULL) {
			return -EFAULT;
		}

		struct fb_var_screeninfo info = { SCREEN_WIDTH, SCREEN_HEIGHT,
			                                SCREEN_BPP_DEPTH };
		memcpy(scr_info, &info, sizeof(struct fb_var_screeninfo));
		return 0;
		break;
	}
	case TIOCGPGRP: {
		if (file->ip->major != DEV_TTY) {
			return -EINVAL;
		}
		pid_t *pgrp;
		PROPOGATE_ERR(argptr(2, (char **)&pgrp, sizeof(pid_t *)));
		if (pgrp == NULL) {
			return -EFAULT;
		}
		*pgrp = get_term_pgid(file->ip->minor);
		return 0;
		break;
	}
	case TIOCGETAW:
	case TIOCGETAF:
	case TIOCGETA: {
		if (file->ip->major != DEV_TTY) {
			return -EINVAL;
		}
		struct termios *termios;
		PROPOGATE_ERR(argptr(2, (char **)&termios, sizeof(struct termios *)));

		if (termios == NULL) {
			return -EFAULT;
		}

		struct termios *ret = get_term_settings(file->ip->minor);
		// Copy here because userspace cannot use kernel pointers.
		memcpy(termios, ret, sizeof(struct termios));
		return 0;
		break;
	}
	case TIOCSPGRP: {
		if (file->ip->major != DEV_TTY) {
			return -EINVAL;
		}
		pid_t pgid;
		PROPOGATE_ERR(argpid_t(2, &pgid));
		if (pgid < 0 || pgid >= NPROC) {
			return -EINVAL;
		}
		set_term_pgid(file->ip->minor, pgid);
		return 0;
		break;
	}
	case TIOCSETAW:
	case TIOCSETAF:
	case TIOCSETA: {
		if (file->ip->major != DEV_TTY) {
			return -ENOTTY;
		}
		struct termios *termios;
		PROPOGATE_ERR(argptr(2, (char **)&termios, sizeof(struct termios *)));

		if (termios == NULL) {
			return -EFAULT;
		}

		set_term_settings(file->ip->minor, termios);
		// Adjust whether chracters print.
		echo_out = (termios->c_lflag & ECHO) == ECHO;

		return 0;
		break;
	}
	case TIOCGSID: {
		pid_t *user_sid;
		PROPOGATE_ERR(argptr(2, (char **)&user_sid, sizeof(pid_t *)));
		// Situations for ENOTTY:
		// "The calling process does not have a controlling terminal, or the file is
		// not the controlling terminal."

		if (file->ip->major != DEV_TTY) {
			return -ENOTTY;
		}
		if (myproc()->ctty != makedev(file->ip->major, file->ip->minor)) {
			return -ENOTTY;
		}
		pid_t sid = get_term_sid(file->ip->minor);
		if (sid == 0) {
			return -ENOTTY;
		}
		// Copy the session ID into user_sid.
		*user_sid = sid;
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

	if (file == NULL) {
		return -ENOENT;
	}
	switch (op) {
	case F_DUPFD: {
		int arg;
		int fd;
		struct file *duped_file;
		PROPOGATE_ERR(argint(2, &arg));
		PROPOGATE_ERR(fd = fdalloc(file));
		if ((duped_file = filedup(file, 0)) == NULL) {
			return -EBADF;
		}
		return fd;
	}
	case F_DUPFD_CLOEXEC: {
		int arg;
		int fd;
		struct file *duped_file;
		PROPOGATE_ERR(argint(2, &arg));
		PROPOGATE_ERR(fd = fdalloc(file));
		if ((duped_file = filedup(file, O_CLOEXEC)) == NULL) {
			return -EBADF;
		}
		return fd;
	}
	case F_DUPFD_CLOFORK: {
		int arg;
		int fd;
		struct file *duped_file;
		PROPOGATE_ERR(argint(2, &arg));
		PROPOGATE_ERR(fd = fdalloc(file));
		if ((duped_file = filedup(file, O_CLOFORK)) == NULL) {
			return -EBADF;
		}
		return fd;
	}
	case F_GETFL: {
		inode_lock(file->ip);
		int flags = file->ip->fattrs;
		inode_unlock(file->ip);
		return flags;
	}
	case F_SETFL: {
		int arg;
		PROPOGATE_ERR(argint(2, &arg));
		inode_lock(file->ip);
		file->ip->fattrs = arg;
		inode_unlock(file->ip);
		return 0;
	}
	case F_GETFD: {
		return file->flags;
	}
	case F_SETFD: {
		int arg;
		PROPOGATE_ERR(argint(2, &arg));
		if (arg & ~(FD_CLOEXEC | FD_CLOFORK)) {
			return -EINVAL;
		}
		int new_arg = 0;
		if (arg & FD_CLOFORK) {
			new_arg |= O_CLOFORK;
		}
		if (arg & FD_CLOEXEC) {
			new_arg |= O_CLOEXEC;
		}
		file->flags = new_arg;
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

#define MMAP_HAS_FLAG(x, flag) ((x & flag) == flag)

size_t
sys_mmap(void)
{
	void *user_virt_addr;
	size_t length;
	int prot, flags, fd;
	struct file *file = NULL;
	off_t offset;
	PROPOGATE_ERR(argptr(0, (char **)&user_virt_addr, sizeof(void *)));
	PROPOGATE_ERR(argsize_t(1, &length));
	PROPOGATE_ERR(argint(2, &prot));
	PROPOGATE_ERR(argint(3, &flags));

	if (!MMAP_HAS_FLAG(flags, MAP_ANONYMOUS)) {
		PROPOGATE_ERR(argfd(4, &fd, &file));
	} else {
		// We ignore whatever is in fd if we map anonymous.
		(void)argfd(4, &fd, &file);
	}

	PROPOGATE_ERR(argoff_t(5, &offset));

	if (length == 0) {
		return -EINVAL;
	}
	// We don't support MAP_PRIVATE for now.
	if (!MMAP_HAS_FLAG(flags, MAP_SHARED) &&
	    !MMAP_HAS_FLAG(flags, MAP_ANONYMOUS)) {
		return -EINVAL;
	}

	if (length % PGSIZE != 0 || (uintptr_t)user_virt_addr % PGSIZE != 0) {
		return -EINVAL;
	}
	int perm = mmap_prot_to_perm(prot);
	struct mmap_info info;
	if (!MMAP_HAS_FLAG(flags, MAP_ANONYMOUS) && file != NULL &&
	    S_ISCHR(file->ip->mode)) {
		// "The file has been locked, or too much memory has been locked"
		if (atomic_load(&file->ip->lock.locked)) {
			return -EAGAIN;
		}
		if (file->ip->major < 0 || file->ip->major >= NDEV ||
		    !devsw[file->ip->major].mmap) {
			return -ENODEV;
		}
		info = devsw[file->ip->major].mmap(file->ip->minor, length,
		                                   (uintptr_t)user_virt_addr, perm);
		info.file = file;
	} else {
		info =
			(struct mmap_info){ length, 0, (uintptr_t)user_virt_addr, NULL, perm };
	}
	struct proc *proc = myproc();
	if (proc->mmap_count > NMMAP) {
		return -ENOMEM;
	}

	// Place it anywhere.
	if (info.virt_addr == 0) {
		// Next spot in the heap.
		info.virt_addr = PGROUNDUP(proc->heap + proc->heapsz);

		uintptr_t user_phys_addr;
		if (info.addr == 0) {
			PROPOGATE_ERR(alloc_user_bytes(myproc()->pgdir, info.length,
			                               info.virt_addr, &user_phys_addr));
			info.addr = user_phys_addr;
		} else {
			PROPOGATE_ERR(mappages(myproc()->pgdir, (void *)info.virt_addr,
			                       info.length, info.addr, info.perm));
		}

		myproc()->heapsz += info.length;
		proc->mmap_info[proc->mmap_count++] = info;
		return (size_t)info.virt_addr;
	} else {
		if (mappages(proc->pgdir, user_virt_addr, info.length, info.addr,
		             info.perm) < 0) {
			// If we arrive here, odds are the physical address we were given is
			// garbage.
			void *ptr = kmalloc(info.length);
			if (ptr == NULL) {
				return -ENOMEM;
			}
			info.addr = V2P(ptr);
			if (mappages(proc->pgdir, user_virt_addr, info.length, info.addr,
			             info.perm) < 0) {
				kfree(ptr);
				return -ENOMEM;
			}
			myproc()->heapsz += info.length;
			proc->mmap_info[proc->mmap_count++] = info;
			return (size_t)ptr;
		}
		myproc()->heapsz += info.length;
		proc->mmap_info[proc->mmap_count++] = info;
		return (size_t)user_virt_addr;
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

	if (length == 0) {
		return -EINVAL;
	}
	int j = -1;
	for (int i = 0; i < NMMAP; i++) {
		if ((proc->mmap_info[i].virt_addr == (uintptr_t)addr) &&
		    ((proc->mmap_info[i].length == length) ||
		     (proc->mmap_info[i].file &&
		      S_ISCHR(proc->mmap_info[i].file->ip->mode)))) {
			j = i;
		}
	}
	if (j == -1) {
		return -EINVAL;
	}
	for (int i = 0; i < PGROUNDUP(proc->mmap_info[j].length); i += PGSIZE) {
		unmap_user_page(proc->pgdir, (char *)proc->mmap_info[j].virt_addr);
	}
	if (addr != NULL && V2P(addr) < KERNBASE) {
		kfree(addr);
	}
	return 0;
}

/* Unimplemented */
size_t
sys_fsync(void)
{
	int fd;
	struct file *file;
	PROPOGATE_ERR(argfd(0, &fd, &file));

	if (fd == 0) {
		return -EINVAL;
	} else if (fd == 1 || fd == 2) {
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
	if (argmode_t(0, &mask) < 0) {
		return S_IWGRP | S_IWOTH;
	}
	return myproc() ? myproc()->umask : (S_IWGRP | S_IWOTH);
}

size_t
sys_renameat(void)
{
	char *oldpath_;
	char *newpath_;
	char dir[DIRSIZ];
	char newdir[DIRSIZ];
	struct inode *ip1, *ip2;
	struct dirent de;
	int fd1;
	int fd2;
	char newelem[DIRSIZ];

	PROPOGATE_ERR(argfd(0, &fd1, NULL));
	PROPOGATE_ERR(argstr(1, &oldpath_));
	PROPOGATE_ERR(argfd(2, &fd2, NULL));
	PROPOGATE_ERR(argstr(3, &newpath_));
	// argstr wants a mutable char *, but our arguments are const.
	// we cast them back here.
	const char *oldpath = oldpath_;
	const char *newpath = newpath_;

	begin_op();
	if ((ip1 = namei(oldpath)) == NULL) {
		end_op();
		return -ENOENT;
	}
	inode_put(ip1);
	if ((ip2 = nameiparent(newpath, newelem)) == NULL) {
		end_op();
		return -ENOENT;
	}
	inode_put(ip2);

	struct inode *dp = nameiparent_with_fd(fd1, oldpath, dir);
	struct inode *new_dp = nameiparent_with_fd(fd2, newpath, newdir);
	if (dp == NULL) {
		if (new_dp != NULL) {
			inode_put(new_dp);
		}
		end_op();
		return -ENOENT;
	}
	if (new_dp == NULL) {
		if (dp != NULL) {
			inode_put(dp);
		}
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
			inode_unlockput(dp);
			inode_put(new_dp);
			return -ENOENT;
		}
		if (strncmp(de.d_name, dir, min(strlen(dir), sizeof(de.d_name))) == 0) {
			uint16_t inode = de.d_ino;

			// Remove the old entry.
			memset(&de, '\0', sizeof(de));
			if (inode_write(dp, (char *)&de, off, sizeof(de)) != sizeof(de)) {
				end_op();
				inode_unlockput(dp);
				inode_put(new_dp);
				return -ENOSPC;
			}

			// In the case of "mv /foo /bar", we use the same directory pointer
			// (inode). In that case, we do not want to try and lock the same inode
			// twice.
			if (new_dp != dp) {
				inode_lock(new_dp);
			}
			// Add the file to the directory.
			// Reuse the inode number, but have a new name.
			if (dirlink(new_dp, newelem, inode) < 0) {
				if (new_dp != dp) {
					inode_unlockput(new_dp);
				}
				return -EEXIST;
			}
			if (new_dp != dp) {
				inode_unlockput(new_dp);
			}

			// Commit to disk.
			inode_unlockput(dp);
			end_op();
			return 0;
		}
	}

	inode_unlockput(dp);
	inode_put(new_dp);
	end_op();
	return -ENOENT;
}

size_t
sys_faccessat(void)
{
	char *path;
	// Not to be confused with the
	// mode_t mode.
	int mode;
	int fd;
	int flags;
	struct inode *ip;
	// Initialized down below.
	bool perms_wanted_ok;
	bool read_ok = false;
	bool write_ok = false;
	bool exec_ok = false;
	bool correct_uid;
	bool correct_gid;
	PROPOGATE_ERR(argfd(0, &fd, NULL));
	PROPOGATE_ERR(argstr(1, &path));
	PROPOGATE_ERR(argint(2, &mode));
	// AT_EACCESS is the only POSIX one and
	// effective UID/GIDs are not implemented yet.
	PROPOGATE_ERR(argint(3, &flags));
	begin_op();
	// File does not exist,
	if ((ip = namei_with_fd(fd, path)) == NULL) {
		end_op();
		return -ENOENT;
	}
	// All we wanted to do is check whether the
	// file exists, and it does.
	if (mode == F_OK) {
		inode_put(ip);
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

	inode_unlockput(ip);
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
