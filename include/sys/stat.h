#pragma once
#include <stat.h>
#include <sys/types.h>
int mknod(const char *, mode_t mode, dev_t device);
int fstat(int fd, struct stat *restrict statbuf);
int mkdir(const char *dir, mode_t mode);
int chmod(const char *, mode_t mode);
int fchmod(int fd, mode_t mode);
__attribute__((nonnull(2))) int stat(const char *restrict pathname,
                                     struct stat *restrict statbuf);
int lstat(const char *restrict n, struct stat *restrict st);
mode_t umask(mode_t mask);
int mkfifo(const char *pathname, mode_t mode);
