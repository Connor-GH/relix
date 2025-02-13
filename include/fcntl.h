#pragma once
#include "kernel/include/fcntl_constants.h"
int
open(const char *, int);
int
fcntl(int fd, int cmd, ...);
