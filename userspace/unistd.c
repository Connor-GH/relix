#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/times.h>
#include <sys/types.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libc_syscalls.h"

extern char **environ;

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

int
pipe(int pipefd[2])
{
	return pipe2(pipefd, 0);
}

int
pipe2(int pipefd[2], int oflags)
{
	return __syscall_ret(__syscall2(SYS_pipe2, (long)pipefd, oflags));
}

ssize_t
read(int fd, void *buf, size_t count)
{
	if (fd < 0) {
		return __syscall_ret(-EBADF);
	}
	return __syscall_ret(__syscall3(SYS_read, fd, (long)buf, count));
}

ssize_t
write(int fd, const void *buf, size_t count)
{
	if (fd < 0) {
		return __syscall_ret(-EBADF);
	}
	return __syscall_ret(__syscall3(SYS_write, fd, (long)buf, count));
}

int
close(int fd)
{
	if (fd < 0) {
		return __syscall_ret(-EBADF);
	}
	return __syscall_ret(__syscall1(SYS_close, fd));
}

int
execve(const char *pathname, char *const argv[], char *const envp[])
{
	return __syscall_ret(
		__syscall3(SYS_execve, (long)pathname, (long)argv, (long)envp));
}

int
execv(const char *prog, char *const *argv)
{
	return execve(prog, argv, (char *const[]){ "", NULL });
}

int
execvp(const char *file, char *const argv[])
{
	char str[PATH_MAX];

	char *path_env = getenv("PATH");
	if (path_env == NULL) {
		// This is implementation-defined.
		// In order to not break a system, we fall back to /bin.
		path_env = "/bin";
	}

	// "If the file argument contains a <slash> character,
	// the file argument shall be used as the pathname
	// for this file." - POSIX.1-2024
	if (strchr(file, '/') != NULL) {
		execve(file, argv, environ);
		return -1;
	}

	char *path = strdup(path_env); // TODO leak of this memory
	if (path != NULL) {
		char *s = strtok(path, ":");
		while (s != NULL) {
			sprintf(str, "%s/%s", s, file);
			errno = 0;
			execve(str, argv, environ);
			s = strtok(NULL, ":");
		}
	}

	return -1;
}

static size_t
va_list_to_array(va_list listp, char *vector[ARG_MAX])
{
	char *val;
	size_t i = 0;
	do {
		val = va_arg(listp, char *);
		vector[i++] = val;
	} while (val != NULL && i < ARG_MAX);
	return i;
}

int
execl(const char *path, const char *arg, ...)
{
	char *argv[ARG_MAX] = {};
	va_list listp;

	va_start(listp, arg);
	(void)va_list_to_array(listp, argv);
	va_end(listp);

	return execv(path, argv);
}

int
execlp(const char *file, const char *arg, ...)
{
	char *argv[ARG_MAX] = {};
	va_list listp;

	va_start(listp, arg);
	(void)va_list_to_array(listp, argv);
	va_end(listp);

	return execvp(file, argv);
}

int
execle(const char *path, const char *arg, ...)
{
	char *argv[ARG_MAX] = {};
	va_list listp;

	va_start(listp, arg);
	size_t i = va_list_to_array(listp, argv);
	va_list_to_array(listp, argv + i);
	va_end(listp);

	return execve(path, argv, argv + i);
}

int
unlinkat(int fd, const char *name, int flag)
{
	return __syscall_ret(__syscall3(SYS_unlinkat, fd, (long)name, flag));
}

int
unlink(const char *pathname)
{
	return unlinkat(AT_FDCWD, pathname, 0);
}

int
link(const char *oldpath, const char *newpath)
{
	return __syscall_ret(__syscall5(SYS_linkat, (long)AT_FDCWD, (long)oldpath,
	                                AT_FDCWD, (long)newpath, 0));
}

int
chdir(const char *path)
{
	return __syscall_ret(__syscall1(SYS_chdir, (long)path));
}

int
dup(int oldfd)
{
	return fcntl(oldfd, F_DUPFD, 0);
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

int
setpgid(pid_t pid, pid_t pgid)
{
	return __syscall_ret(__syscall2(SYS_setpgid, pid, pgid));
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

pid_t
getpgid(pid_t pid)
{
	return __syscall_ret(__syscall1(SYS_getpgid, pid));
}

pid_t
getpgrp(void)
{
	return getpgid(0);
}

int
symlinkat(const char *from, int tofd, const char *to)
{
	return __syscall_ret(__syscall3(SYS_symlinkat, (long)from, tofd, (long)to));
}

int
symlink(const char *target, const char *linkpath)
{
	return symlinkat(target, AT_FDCWD, linkpath);
}

ssize_t
readlinkat(int dirfd, const char *restrict pathname, char *restrict linkpath,
           size_t bufsiz)
{
	return __syscall_ret(
		__syscall4(SYS_readlinkat, dirfd, (long)pathname, (long)linkpath, bufsiz));
}
ssize_t
readlink(const char *restrict pathname, char *restrict linkpath, size_t bufsiz)
{
	return readlinkat(AT_FDCWD, pathname, linkpath, bufsiz);
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
		if (n == 0) {
			alloc_size = PATH_MAX;
		} else {
			alloc_size = n;
		}

		buf = malloc(alloc_size);
		if (buf == NULL) {
			return NULL;
		}

		return buf;
	}

	int ret = __syscall_ret(__syscall2(SYS_getcwd, (long)buf, n));
	if (ret < 0) {
		return NULL;
	} else {
		return buf;
	}
}

__deprecated("Removed in POSIX.1-2008; use fork()") pid_t vfork(void)
{
	errno = ENOSYS;
	return -1;
}

clock_t
times(struct tms *buf)
{
	return __syscall_ret(__syscall1(SYS_times, (long)buf));
}

int
faccessat(int fd, const char *pathname, int mode, int flags)
{
	return __syscall_ret(
		__syscall4(SYS_faccessat, fd, (long)pathname, mode, flags));
}

int
access(const char *pathname, int mode)
{
	return faccessat(AT_FDCWD, pathname, mode, 0);
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

static char ttyname_buf[TTY_NAME_MAX];

char *
ttyname(int fd)
{
	int ret = ttyname_r(fd, ttyname_buf, sizeof(ttyname_buf));
	if (ret < 0) {
		return NULL;
	}
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
	if (dir == NULL) {
		return -1;
	}
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

int
dup3(int oldfd, int newfd, int flags)
{
	return __syscall_ret(__syscall3(SYS_dup3, oldfd, newfd, flags));
}

int
dup2(int oldfd, int newfd)
{
	return dup3(oldfd, newfd, 0);
}

int
isatty(int fd)
{
	return fd == 0;
}

static void
sleep_signal_noop(int signum)
{
}

unsigned int
sleep(unsigned int seconds)
{
	signal(SIGALRM, sleep_signal_noop);
	return alarm(seconds);
}

int
rmdir(const char *path)
{
	if (strncmp(path, "/", 1) == 0) {
		errno = EBUSY;
		return -1;
	}
	char buf[PATH_MAX];
	if (getcwd(buf, sizeof(buf)) == NULL) {
		return -1;
	}
	if (strncmp(path, buf, strlen(path)) == 0) {
		errno = EBUSY;
		return -1;
	}
	struct stat st;
	if (stat(path, &st) < 0) {
		return -1;
	}
	if (S_ISLNK(st.st_mode)) {
		errno = ENOTDIR;
		return -1;
	}
	// TODO handle last dot and dot-dot.
	// TODO handle non-empty directory.
	return unlink(path);
}
