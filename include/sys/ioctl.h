#pragma once
#include "kernel/include/ioctl.h"
int ioctl(int fd, unsigned long request, ...);
