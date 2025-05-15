#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>

#include "libc_syscalls.h"

pid_t
fork(void)
{
	return __syscall_ret(__syscall0(SYS_fork));
}


void __noreturn
_exit(int status)
{
	__syscall_ret(__syscall1(SYS__exit, status));
	unreachable();
}

pid_t
wait(int *wstatus)
{
	return __syscall_ret(__syscall1(SYS_wait, (long)wstatus));
}

int
pipe(int pipefd[2])
{
	return __syscall_ret(__syscall1(SYS_pipe, (long)pipefd));
}

ssize_t
read(int fd, void *buf, size_t count)
{
	if (fd < 0)
		return __syscall_ret(-EBADF);
	return __syscall_ret(__syscall3(SYS_read, fd, (long)buf, count));
}

ssize_t
write(int fd, const void *buf, size_t count)
{
	if (fd < 0)
		return __syscall_ret(-EBADF);
	return __syscall_ret(__syscall3(SYS_write, fd, (long)buf, count));
}

int
close(int fd)
{
	if (fd < 0)
		return __syscall_ret(-EBADF);
	return __syscall_ret(__syscall1(SYS_close, fd));
}


int
execve(const char *pathname, char *const argv[], char *const envp[])
{
	return __syscall_ret(__syscall3(SYS_execve, (long)pathname, (long)argv, (long)envp));
}


int
unlink(const char *pathname)
{
	return __syscall_ret(__syscall1(SYS_unlink, (long)pathname));
}

int
link(const char *oldpath, const char *newpath)
{
	return __syscall_ret(__syscall2(SYS_link, (long)oldpath, (long)newpath));
}

int
chdir(const char *path)
{
	return __syscall_ret(__syscall1(SYS_chdir, (long)path));
}

int
dup(int oldfd)
{
	return __syscall_ret(__syscall1(SYS_dup, oldfd));
}

pid_t
getpid(void)
{
	return __syscall_ret(__syscall0(SYS_getpid));
}

pid_t
getppid(void)
{
	return __syscall_ret(__syscall0(SYS_getppid));
}

void *
sbrk(intptr_t increment)
{
	return (void *)__syscall_ret(__syscall1(SYS_sbrk, increment));
}

unsigned int
alarm(unsigned int seconds)
{
	return __syscall_ret(__syscall1(SYS_alarm, seconds));
}

time_t
time(time_t *tloc)
{
	return __syscall_ret(__syscall1(SYS_time, (long)tloc));
}

int
setgid(gid_t gid)
{
	return __syscall_ret(__syscall1(SYS_setgid, gid));
}

int
setuid(uid_t uid)
{
	return __syscall_ret(__syscall1(SYS_setuid, uid));
}

uid_t
getuid(void)
{
	return __syscall_ret(__syscall0(SYS_getuid));
}

gid_t
getgid(void)
{
	return __syscall_ret(__syscall0(SYS_getgid));
}


int
symlink(const char *target, const char *linkpath)
{
	return __syscall_ret(__syscall2(SYS_symlink, (long)target, (long)linkpath));
}

ssize_t
readlink(const char *restrict pathname, char *restrict linkpath, size_t buf)
{
	return __syscall_ret(__syscall3(SYS_readlink, (long)pathname, (long)linkpath, buf));
}

off_t
lseek(int fd, off_t offset, int whence)
{
	return __syscall_ret(__syscall3(SYS_lseek, fd, offset, whence));
}

int
fsync(int fd)
{
	return __syscall_ret(__syscall1(SYS_fsync, fd));
}

char *
getcwd(char *buf, size_t n)
{
	// POSIX leaves this the behavior on NULL buf
	// unspecified, but bash needs this in order to
	// not deref a NULL pointer. Objectively, this
	// is bash's fault for not following the standard,
	// but I will make an exception this *once*.
	if (buf == NULL) {
		size_t alloc_size;
		if (n == 0)
			alloc_size = PATH_MAX;
		else
			alloc_size = n;

		buf = malloc(alloc_size);
		if (buf == NULL)
			return NULL;

		return buf;
	}

	int ret = __syscall_ret(__syscall2(SYS_getcwd, (long)buf, n));
	if (ret < 0)
		return NULL;
	else
		return buf;
}

__deprecated("Removed in POSIX.1-2008; use fork()") pid_t
vfork(void)
{
	return __syscall_ret(__syscall0(SYS_vfork));
}

clock_t
times(struct tms *buf)
{
	return __syscall_ret(__syscall1(SYS_times, (long)buf));
}

int
access(const char *pathname, int mode)
{
	return __syscall_ret(__syscall2(SYS_access, (long)pathname, mode));
}

uid_t
geteuid(void)
{
	return getuid();
}

uid_t
getegid(void)
{
	return getgid();
}

static char ttyname_buf[FILENAME_MAX];

char *
ttyname(int fd)
{
	int ret = ttyname_r(fd, ttyname_buf, sizeof(ttyname_buf));
	if (ret < 0)
		return NULL;
	return ttyname_buf;
}

int
ttyname_r(int fd, char *buf, size_t buflen)
{
	struct stat fd_stat;
	// Stat the fd to get the inode number.
	if (fstat(fd, &fd_stat) < 0) {
		return -1;
	}
	// Go through /dev to find devices (for tty).
	DIR *dir = opendir("/dev");
	if (dir == NULL)
		return -1;
	struct dirent *d;
	while ((d = readdir(dir)) != NULL) {
		// We found the right file. Put its pathname in buf.
		if (d->d_ino == fd_stat.st_ino) {
			snprintf(buf, buflen, "%s/%s", "/dev", d->d_name);
			closedir(dir);
			return 0;
		}
	}
	closedir(dir);
	return -1;
}
