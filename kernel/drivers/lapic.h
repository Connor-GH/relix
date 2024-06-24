#pragma once
#include <types.h>
#include <date.h>
extern volatile uint *lapic;

void
cmostime(struct rtcdate *r);
int
lapicid(void);
void
lapiceoi(void);
void
lapicinit(void);
void
lapicstartap(uchar a, uint b);
void
microdelay(int);
