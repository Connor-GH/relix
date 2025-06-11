#pragma once
#if __KERNEL__
#include "proc.h"
#include "time.h"
extern time_t ticks;
extern struct spinlock tickslock;
void regdump(struct trapframe *tf);
#endif
