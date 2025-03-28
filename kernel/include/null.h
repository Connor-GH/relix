#pragma once
#if __KERNEL__
#include "fs.h"

void
nulldrvinit(void);
int
nulldrvread(struct inode *ip, char *buf, int n);
int
nulldrvwrite(struct inode *ip, char *buf, int n);
#endif
