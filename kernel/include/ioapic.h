#pragma once
#if __RELIX_KERNEL__
#include <stdint.h>
void ioapicenable(int irq, int cpu);
extern uint8_t ioapicid;
void ioapicinit(void);
#endif
