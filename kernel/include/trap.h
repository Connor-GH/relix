#pragma once
#include <types.h>
void
idtinit(void);
extern uint ticks;
void
tvinit(void);
extern struct spinlock tickslock;
