//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "fb.h"
#include "fcntl_constants.h"
#include "memlayout.h"
#include "mmu.h"
#include "pci.h"
#include "vga.h"
#include <defs.h>
#include <stdint.h>
#include <stat.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <dirent.h>
#include <date.h>
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

	if (argint(n, &fd) < 0)
		return -EINVAL;
	if (fd < 0 || (f = myproc()->ofile[fd]) == 0)
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
		if (curproc->ofile[fd] == 0) {
			curproc->ofile[fd] = f;
			return fd;
		}
	}
	return -1;
}

size_t
sys_dup(void)
{
	struct file *f;
	int fd;

	if (argfd(0, 0, &f) < 0)
		return -1;
	if ((fd = fdalloc(f)) < 0)
		return -EBADF;
	filedup(f);
	return fd;
}

size_t
sys_read(void)
{
	struct file *f;
	uint64_t n;
	char *p;

	// do not rearrange, because then 'n' will be undefined.
	if (argfd(0, 0, &f) < 0 || arguintptr_t(2, &n) < 0 || argptr(1, &p, n) < 0)
		return -EINVAL;
	return fileread(f, p, n);
}

size_t
sys_write(void)
{
	struct file *f;
	uint64_t n;
	char *p;

	if (argfd(0, 0, &f) < 0 || arguintptr_t(2, &n) < 0 || argptr(1, &p, n) < 0)
		return -EINVAL;
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
	if (argfd(0, &fd, &file) < 0 ||
			argptr(1, (void *)&iovecs, sizeof(*iovecs)) < 0 ||
			argint(2, &iovcnt) < 0) {
		return -EINVAL;
	}
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

	if (argfd(0, &fd, &f) < 0)
		return -EINVAL;
	myproc()->ofile[fd] = 0;
	fileclose(f);
	return 0;
}

size_t
sys_fstat(void)
{
	struct file *f;
	struct stat *st;

	if (argfd(0, 0, &f) < 0 || argptr(1, (void *)&st, sizeof(*st)) < 0)
		return -EINVAL;
	if (st == NULL)
		return -EINVAL;
	return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
size_t
sys_link(void)
{
	char name[DIRSIZ], *new, *old;
	int retflag = EINVAL;
	struct inode *dp, *ip;

	if (argstr(0, &old) < 0 || argstr(1, &new) < 0)
		return -EINVAL;

	begin_op();
	if ((ip = namei(old)) == 0) {
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

	if ((dp = nameiparent(new, name)) == 0) {
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

	if (argstr(0, &path) < 0)
		return -EINVAL;

	begin_op();
	if ((dp = nameiparent(path, name)) == 0) {
		end_op();
		return -ENOENT;
	}

	inode_lock(dp);

	// Cannot unlink "." or "..".
	if (namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
		goto bad;

	if ((ip = dirlookup(dp, name, &off)) == 0) {
		error = ENOENT;
		goto bad;
	}

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

	// get inode of path, and put the name in name.
	if ((dp = nameiparent(path, name)) == 0)
		return 0;
	inode_lock(dp);

	if ((ip = dirlookup(dp, name, 0)) != 0) {
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
		if (S_ISREG(ip->mode) && S_ISREG(mode)) {
			ip->mode = mode;
			return ip;
		}
		inode_unlockput(ip);
		return 0;
	}

	if ((ip = inode_alloc(dp->dev, mode)) == 0)
		panic("create: inode_alloc");

	inode_lock(ip);
	ip->major = major;
	ip->minor = minor;
	ip->nlink = 1;
	ip->mode = mode;
	ip->gid = DEFAULT_GID;
	ip->uid = DEFAULT_UID;
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
		if ((ip = namei(path)) != 0) {
			// if it's a block device, possibly do something special.
			inode_lock(ip);
			if (S_ISBLK(ip->mode)) {
				goto get_fd;
			}
			// if it's not a block device, just exit.
			inode_unlockput(ip);
			end_op();
			return -ENOTBLK;
		}
		// create() holds a lock on this inode pointer,
		// but only if it succeeds.
		ip = create(path, mode, 0, 0);
		if (ip == 0) {
			end_op();
			return -EIO;
		}
		if (!S_ISANY(ip->mode)) {
			ip->mode |= S_IFREG;
		}
	} else {
		if ((ip = namei(path)) == 0) {
			end_op();
			return -ENOENT;
		}
		inode_lock(ip);

		if (S_ISLNK(ip->mode)) {
			if ((ip = link_dereference(ip, path)) == 0) {
				inode_unlockput(ip);
				end_op();
				return -EINVAL;
			}
		}
		if (S_ISDIR(ip->mode) && flags != O_RDONLY) {
			inode_unlockput(ip);
			end_op();
			return -EISDIR;
		}
		if (S_ISBLK(ip->mode)) {
			if (ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].open) {
				inode_unlockput(ip);
				end_op();
				return -ENODEV;
			}
			// Run device-specific opening code, if any.
			devsw[ip->major].open(ip->minor, flags);
		}
	}
	// By this line, both branches above are holding a lock to ip.
	// That is why it is released down here.
get_fd:

	if ((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0) {
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
	f->off = (flags & O_APPEND) == O_APPEND ? f->ip->size : 0;
	f->readable = !(flags & O_WRONLY);
	f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
	return fd;
}

size_t
sys_open(void)
{
	char *path;
	int flags;
	mode_t mode;

	if (argstr(0, &path) < 0 || argint(1, &flags) < 0)
		return -EINVAL;
	if (((flags & O_CREAT) == O_CREAT) || ((flags & O_TMPFILE) == O_TMPFILE)) {
		if (argmode_t(2, &mode) < 0)
			return -EINVAL;
	} else {
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

	if (argstr(0, &path) < 0 || argint(1, &mode) < 0) {
		return -EINVAL;
	}
	if (myproc() == NULL)
		return -EAGAIN;
	begin_op();
	if ((ip = create(path, (S_IFDIR | mode) & ~myproc()->umask, 0, 0)) == 0) {
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

	if ((argstr(0, &path)) < 0 || argint(1, &mode) < 0 ||
			arguintptr_t(2, &dev) < 0) {
		return -EINVAL;
	}
	begin_op();
	if ((ip = create(path, S_IFBLK | mode, major(dev), minor(dev))) == 0) {
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

	begin_op();
	if (argstr(0, &path) < 0 || (ip = namei(path)) == 0) {
		end_op();
		return -EINVAL;
	}
	inode_lock(ip);
	if (S_ISLNK(ip->mode)) {
		if ((ip = link_dereference(ip, path)) == 0) {
			end_op();
			panic("open link_dereference");
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
	if (argstr(0, &buf) < 0 || argsize_t(1, &size) < 0)
		return -EINVAL;

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

	if (argstr(0, &path) < 0 || arguintptr_t(1, &uargv) < 0 ||
			arguintptr_t(2, &uenvp) < 0) {
		return -EINVAL;
	}
	memset(argv, 0, sizeof(argv));
	memset(envp, 0, sizeof(envp));
	for (size_t i = 0;; i++) {
		if (i >= NELEM(argv))
			return -EINVAL;
		if (fetchuintptr_t(uargv + sizeof(uintptr_t) * i, &uarg) < 0)
			return -EINVAL;
		if (uarg == 0) {
			argv[i] = 0;
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
			envp[i] = 0;
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
	int *fd;
	struct file *rf, *wf;
	int fd0, fd1;

	if (argptr(0, (void *)&fd, 2 * sizeof(fd[0])) < 0)
		return -EINVAL;
	if (pipealloc(&rf, &wf) < 0)
		return -EINVAL;
	fd0 = -1;
	if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0) {
		if (fd0 >= 0)
			myproc()->ofile[fd0] = 0;
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
	begin_op();
	if (argstr(0, &path) < 0 || argint(1, (mode_t *)&mode) < 0 ||
			(ip = namei(path)) == 0) {
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
sys_echoout(void)
{
	int answer;
	begin_op();
	if (argint(0, &answer) < 0) {
		end_op();
		return -EINVAL;
	}
	echo_out = answer;
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
	if (argstr(0, &target) < 0 || argstr(1, &linkpath) < 0)
		return -EINVAL;

	begin_op();
	if ((eexist = namei(linkpath)) != 0) {
		end_op();
		return -EEXIST;
	}
	if ((eexist = nameiparent(linkpath, dir)) == 0) {
		end_op();
		return -ENOENT;
	}

	// Dirlookup's first arg needs a lock.
	inode_lock(eexist);

	if ((ip = dirlookup(eexist, dir, &poff)) != 0) {
		inode_unlockput(eexist);
		end_op();
		return -EEXIST;
	}
	inode_unlock(eexist);

	if ((ip = create(linkpath, S_IFLNK | S_IAUSR, 0, 0)) == 0) {
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
	if (argstr(0, &target) < 0 || argstr(1, &ubuf) < 0 ||
			argsize_t(2, &bufsize) < 0) {
		return -EINVAL;
	}
	struct inode *ip;
	begin_op();
	if ((ip = namei(target)) == 0) {
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

struct inode *
link_dereference(struct inode *ip, char *buff)
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
		if ((new_ip = namei(buff)) == 0)
			return 0;

		inode_lock(new_ip);
	}
	return new_ip;

bad:
	inode_unlock(new_ip);
	return 0;
}

size_t
sys_lseek(void)
{
	int fd;
	off_t offset;
	int whence;
	struct file *file;

	if (argfd(0, &fd, &file) < 0 || argssize_t(1, &offset) < 0 ||
			argint(2, &whence) < 0)
		return -EINVAL;
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
	void *last_optional_arg = NULL;
	uintptr_t uptr;
	if (argfd(0, &fd, &file) < 0 || argunsigned_long(1, &request) < 0)
		return -EINVAL;

	// The file needs to be a block device.
	if (!S_ISBLK(file->ip->mode))
		return -ENOTTY;

	switch (request) {
	case PCIIOCGETCONF: {
		if (argptr(2, (char **)&last_optional_arg, sizeof(struct pci_conf *)) < 0)
			return -EINVAL;
		if (last_optional_arg == NULL)
			return -EFAULT;

		// INVARIANT: pci_init must happen before pci_get_conf().
		struct FatPointerArray_pci_conf pci_conf = pci_get_conf();
		memcpy(last_optional_arg, pci_conf.ptr,
					 pci_conf.len * sizeof(struct pci_conf));
		return 0;
		break;
	}
	case FBIOGET_VSCREENINFO: {
		if (argptr(2, (char **)&last_optional_arg, sizeof(struct fb_var_screeninfo *)) < 0)
				return -EINVAL;

		if (last_optional_arg == NULL)
			return -EFAULT;

		struct fb_var_screeninfo info = {WIDTH, HEIGHT, BPP_DEPTH};
		memcpy(last_optional_arg, &info, sizeof(struct fb_var_screeninfo));
		return 0;
		break;
	}
	default: {
		return -EINVAL;
	}
	}
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
	if (argptr(0, (char **)&addr, sizeof(void *)) < 0 ||
			argsize_t(1, &length) < 0 || argint(2, &prot) < 0 ||
			argint(3, &flags) < 0 || argfd(4, &fd, &file) < 0 ||
			argoff_t(5, &offset) < 0)
		return -EINVAL;
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
	if (S_ISBLK(file->ip->mode)) {
		if (file->ip->major < 0 || file->ip->major >= NDEV ||
				!devsw[file->ip->major].mmap)
			return -ENODEV;
		info = devsw[file->ip->major].mmap(file->ip->minor, length, (uintptr_t)addr, perm);
		info.file = file;
	} else {
		info = (struct mmap_info){ length, (uintptr_t)addr, 0/* virtual address */, NULL, perm};
	}
	struct proc *proc = myproc();
	if (proc->mmap_count > NMMAP)
		return -ENOMEM;
	// Place it anywhere.
	if (addr == NULL) {
		if (info.virt_addr == 0)
			info.virt_addr = PGROUNDUP(proc->effective_largest_sz);
		if (mappages(myproc()->pgdir, (void *)info.virt_addr,
								 info.length, info.addr, info.perm) < 0) {
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
	if (argptr(0, (char **)&addr, sizeof(void *)) < 0 ||
			 argsize_t(1, &length) < 0)
		return -EINVAL;
	if (length == 0)
		return -EINVAL;
	int j = -1;
	for (int i = 0; i < NMMAP; i++) {
		if ((proc->mmap_info[i].virt_addr == (uintptr_t)addr) &&
        ((proc->mmap_info[i].length == length) ||
        (proc->mmap_info[i].file && S_ISBLK(proc->mmap_info[i].file->ip->mode)))) {
			j = i;
		}
	}
	if (j == -1)
		return -EINVAL;
	for (int i = 0; i < PGROUNDUP(proc->mmap_info[j].length); i+= PGSIZE) {
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
	if (argfd(0, &fd, &file) < 0)
		return -EINVAL;

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
	if (argstr(0, &oldpath_) < 0 || argstr(1, &newpath_) < 0) {
		return -EINVAL;
	}
	// argstr wants a mutable char *, but our arguments are const.
	// we cast them back here.
	const char *oldpath = oldpath_;
	const char *newpath = newpath_;

	begin_op();
	if ((ip1 = namei(oldpath)) == 0) {
		end_op();
		return -ENOENT;
	}
	if ((ip2 = nameiparent(newpath, newelem)) == 0) {
		end_op();
		return -ENOENT;
	}
	struct inode *dp = nameiparent(oldpath, dir);
	struct inode *new_dp = nameiparent(newpath, newdir);
	if (dp == 0 || new_dp == 0) {
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
