#pragma once
#if __KERNEL__
#include "time.h"
#include "proc.h"
extern time_t ticks;
extern struct spinlock tickslock;
void
regdump(struct trapframe *tf);
#endif
