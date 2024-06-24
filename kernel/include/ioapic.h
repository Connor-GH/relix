#pragma once
#include <types.h>
void
ioapicenable(int irq, int cpu);
extern uchar ioapicid;
void
ioapicinit(void);

