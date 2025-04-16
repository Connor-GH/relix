#pragma once
#include <stat.h>
#include <sys/types.h>
int
mknod(const char *, mode_t mode, dev_t device);
int
fstat(int fd, struct stat *);
int
mkdir(const char *dir, mode_t mode);
int
chmod(const char *, mode_t mode);
int
fchmod(int fd, mode_t mode);
__attribute__((nonnull(2))) int
stat(const char *, struct stat *);
int
lstat(const char *n, struct stat *st);
mode_t
umask(mode_t mask);
int
mkfifo(const char *pathname, mode_t mode);
