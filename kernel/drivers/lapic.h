#pragma once
#include <stdint.h>
#include <date.h>
#include "compiler_attributes.h"
extern volatile uint32_t *lapic;

void
cmostime(struct rtcdate *r);
int
lapicid(void);
void
lapiceoi(void);
void
lapicinit(void);
__suppress_sanitizer("alignment") void lapicstartap(uint8_t a, uint32_t b);
void
microdelay(int);
