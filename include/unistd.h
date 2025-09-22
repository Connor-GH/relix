#pragma once

#include <bits/__NULL.h>
#include <bits/access_constants.h>
#include <bits/seek_constants.h>
#include <bits/size_t.h>
#include <bits/stdint.h>
#include <bits/types.h>

typedef __ssize_t ssize_t;
typedef __size_t size_t;
typedef __pid_t pid_t;
typedef __uid_t uid_t;
typedef __gid_t gid_t;
typedef __off_t off_t;
typedef __intptr_t intptr_t;
#define NULL __NULL

int fork(void) __attribute__((returns_twice));
int vfork(void) __attribute__((returns_twice));
void _exit(int) __attribute__((noreturn));
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int oflags);

int execve(const char *, char *const *, char *const *);
int execvp(const char *file, char *const *argv);
int execv(const char *prog, char *const *argv);
int execl(const char *path, const char *arg, ...);
int execlp(const char *file, const char *arg, ...);
int execle(const char *path, const char *arg, ...);

ssize_t write(int, const void *, size_t);
ssize_t read(int, void *, size_t);
int close(int fd);
int unlink(const char *pathname);
int unlinkat(int dirfd, const char *pathname, int flags);
int rmdir(const char *path);
int link(const char *old, const char *new);
int linkat(int olddirfd, const char *old, int newdirfd, const char *new);
int symlink(const char *target, const char *linkpath);
int symlinkat(const char *from, int tofd, const char *to);
ssize_t readlink(const char *restrict pathname, char *restrict linkpath,
                 size_t bufsiz);
ssize_t readlinkat(int dirfd, const char *restrict pathname,
                   char *restrict linkpath, size_t bufsiz);
int chdir(const char *);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int dup3(int oldfd, int newfd, int flags);

unsigned int alarm(unsigned int);

pid_t getpid(void);
pid_t getppid(void);
pid_t getpgid(pid_t pid);
pid_t getpgrp(void);
gid_t getgid(void);
gid_t getegid(void);
uid_t getuid(void);
uid_t geteuid(void);

int setuid(uid_t uid);
int setgid(gid_t gid);
int setpgid(pid_t pid, pid_t pgid);

void *sbrk(intptr_t increment);
unsigned int sleep(unsigned int);
// needs sys/reboot
int reboot(int cmd);
int fsync(int fd);
extern char *optarg;
extern int optind, opterr, optopt;
int getopt(int argc, char *const argv[], const char *optstring);
char *getcwd(char *buf, size_t size);

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
// POSIX specifies a value other than -1.
#define _POSIX_VDISABLE 0

#define MB_CUR_MAX 1

off_t lseek(int fd, off_t offset, int whence);
int access(const char *pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flags);
int isatty(int fd);
int ttyname_r(int fd, char *buf, size_t buflen);
char *ttyname(int fd);

int getlogin_r(char *name, size_t size);
char *getlogin(void);
