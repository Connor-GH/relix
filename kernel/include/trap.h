#pragma once
#include <stdint.h>
void
idtinit(void);
extern uint32_t ticks;
void
tvinit(void);
extern struct spinlock tickslock;
