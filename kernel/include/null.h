#pragma once
#include "file.h"

void
nulldrvinit(void);
int
nulldrvread(struct inode *ip, char *buf, int n);
int
nulldrvwrite(struct inode *ip, char *buf, int n);
