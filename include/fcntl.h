#pragma once
#include <bits/fcntl_constants.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// POSIX.1-2024: "Inclusion of the <fcntl.h> header may also make visible
// all symbols from <sys/stat.h> and <unistd.h>."
int open(const char *, int flags, ...);
int creat(const char *, mode_t mode);
int fcntl(int fd, int cmd, ...);
