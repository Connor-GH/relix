#pragma once
#include "kernel/include/fcntl_constants.h"
int
open(const char *, int flags, ...);
int
fcntl(int fd, int cmd, ...);
