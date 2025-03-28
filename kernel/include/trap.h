#pragma once
#if __KERNEL__
#include "time.h"
void
idtinit(void);
extern time_t ticks;
void
tvinit(void);
extern struct spinlock tickslock;
#endif
