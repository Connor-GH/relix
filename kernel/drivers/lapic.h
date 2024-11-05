#pragma once
#include <stdint.h>
#include <date.h>
extern volatile uint32_t *lapic;

void
cmostime(struct rtcdate *r);
int
lapicid(void);
void
lapiceoi(void);
void
lapicinit(void);
void
lapicstartap(uint8_t a, uint32_t b);
void
microdelay(int);
