#pragma once
#include <bits/fcntl_constants.h>
#include <sys/types.h>
int open(const char *, int flags, ...);
int creat(const char *, mode_t mode);
int fcntl(int fd, int cmd, ...);
