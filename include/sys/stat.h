#pragma once
#include <stat.h>
int
mknod(const char *, short, short);
int
fstat(int fd, struct stat *);
int
mkdir(const char *);
int
chmod(char *, int mode);
__attribute__((nonnull(2))) int
stat(const char *, struct stat *);
int
lstat(const char *n, struct stat *st);
