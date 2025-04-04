#pragma once
#include <stdint.h>
#include <time.h>
#include "compiler_attributes.h"
extern volatile uint32_t *lapic;

// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID (0x0020 / 4) // ID
#define VER (0x0030 / 4) // Version
#define TPR (0x0080 / 4) // Task Priority
#define EOI (0x00B0 / 4) // EOI
#define SVR (0x00F0 / 4) // Spurious Interrupt Vector
#define ENABLE 0x00000100 // Unit Enable
#define ESR (0x0280 / 4) // Error Status
#define ICRLO (0x0300 / 4) // Interrupt Command
#define INIT 0x00000500 // INIT/RESET
#define STARTUP 0x00000600 // Startup IPI
#define DELIVS 0x00001000 // Delivery status
#define ASSERT 0x00004000 // Assert interrupt (vs deassert)
#define DEASSERT 0x00000000
#define LEVEL 0x00008000 // Level triggered
#define BCAST 0x00080000 // Send to all APICs, including self.
#define BUSY 0x00001000
#define FIXED 0x00000000
#define ICRHI (0x0310 / 4) // Interrupt Command [63:32]
#define TIMER (0x0320 / 4) // Local Vector Table 0 (TIMER)
// Figure 12-10 says that bit 2 is always zero, so this is really 1011.
#define X1 0x0000000B // divide counts by 1
#define PERIODIC 0x00020000 // Periodic
#define ONE_SHOT 0
// may want to use the deadline timer later for realtime applications
#define TSC_DEADLINE 0x40000
#define LVT_TIMER (0x320 / 4) // LVT Timer Register
#define LVT_THERMAL (0x330 / 4) // thermal sensor register
#define PCINT (0x0340 / 4) // Performance Counter LVT
#define LINT0 (0x0350 / 4) // Local Vector Table 1 (LINT0)
#define LINT1 (0x0360 / 4) // Local Vector Table 2 (LINT1)
#define ERROR (0x0370 / 4) // Local Vector Table 3 (ERROR)
#define MASKED 0x00010000 // Interrupt masked
#define TICR (0x0380 / 4) // Timer Initial Count
#define TCCR (0x0390 / 4) // Timer Current Count
#define TDCR (0x03E0 / 4) // Timer Divide Configuration
time_t
rtc_now(void);
int
lapicid(void);
void
lapiceoi(void);
void
lapicinit(void);
__suppress_sanitizer("alignment") void lapicstartap(uint8_t a, uint32_t b);
void
lapicw(int index, int value);
void
microdelay(int);
