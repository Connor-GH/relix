#pragma once

#include <time.h>
#include <stddef.h>
#include <sys/types.h>
#include "kernel/include/lseek.h"
int
fork(void) __attribute__((returns_twice));
int
vfork(void) __attribute__((returns_twice));
void
_exit(int) __attribute__((noreturn));
int
pipe(int *);
int
execve(const char *, char *const *, char *const *);
static inline int
exec(const char *prog, char *const *argv)
{
	return execve(prog, argv, (char *const []){ "", NULL });
}
int
execvp(const char *file, char *const *argv);
// our exec() is technically execv()
static inline int
execv(const char *prog, char *const *argv)
{
	return exec(prog, argv);
}
ssize_t
write(int, const void *, size_t);
ssize_t
read(int, void *, size_t);
int
close(int);
int
unlink(const char *);
int
link(const char *, const char *);
int
symlink(const char *target, const char *linkpath);
ssize_t
readlink(const char *restrict pathname, char *restrict linkpath, size_t buf);
int
chdir(const char *);
int
dup(int);
unsigned int
alarm(unsigned int);

pid_t
getpid(void);
pid_t
getppid(void);

uid_t
getuid(void);
int
setuid(uid_t);
uid_t
geteuid(void);

gid_t
getgid(void);
gid_t
getegid(void);
int
setgid(gid_t);

void *
sbrk(int);
unsigned int
sleep(unsigned int);
// needs sys/reboot
int
reboot(int cmd);
int
fsync(int fd);
extern char *optarg;
extern int optind, opterr, optopt;
int
getopt(int argc, char *const argv[], const char *optstring);
char *
getcwd(char *buf, size_t size);
int
dup2(int oldfd, int newfd);


#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
// POSIX specifies a value other than -1.
#define _POSIX_VDISABLE 0
off_t
lseek(int fd, off_t offset, int whence);
int
isatty(int fd);
int
ttyname_r(int fd, char *buf, size_t buflen);
char *
ttyname(int fd);
