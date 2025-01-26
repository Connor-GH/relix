// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include <stdint.h>
#include <stddef.h>
#include <date.h>
#include "traps.h"
#include "x86.h"
#include "memlayout.h"
#include "stdbool.h"
#include "compiler_attributes.h"
#include "kernel_string.h"
#include "console.h"
#include "spinlock.h"

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
#define X1 0x0000000B // divide counts by 1
#define PERIODIC 0x00020000 // Periodic
// may want to use the deadline timer later for realtime applications
#define TSC_DEADLINE 0x40000
#define LVT_THERMAL (0x330 / 4) // thermal sensor register
#define PCINT (0x0340 / 4) // Performance Counter LVT
#define LINT0 (0x0350 / 4) // Local Vector Table 1 (LINT0)
#define LINT1 (0x0360 / 4) // Local Vector Table 2 (LINT1)
#define ERROR (0x0370 / 4) // Local Vector Table 3 (ERROR)
#define MASKED 0x00010000 // Interrupt masked
#define TICR (0x0380 / 4) // Timer Initial Count
#define TCCR (0x0390 / 4) // Timer Current Count
#define TDCR (0x03E0 / 4) // Timer Divide Configuration

volatile uint32_t *lapic; // Initialized in mp.c

struct spinlock rtc_lock;

static void
lapicw(int index, int value)
{
	lapic[index] = value;
	lapic[ID]; // wait for write to finish, by reading
}

void
lapicinit(void)
{
	if (!lapic)
		return;

	// Enable local APIC; set spurious interrupt vector.
	lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.
	// If xv6 cared more about precise timekeeping,
	// TICR would be calibrated using an external time source.
	uint32_t eax, ebx, ecx, edx;
	cpuid(6, 0, &eax, &ebx, &ecx, &edx);
	// considered the "ARAT" bit
	if (eax & 0b100) {
		// APIC runs at constant rate regardless of P-states
	} else {
		// APIC may stop during C-states or due to intel SpeedStep
	}
	lapicw(TDCR, X1);
	lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
	lapicw(TICR, 1000000);

	// Disable logical interrupt lines.
	lapicw(LINT0, MASKED);
	lapicw(LINT1, MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapic[VER] >> 16) & 0xFF) >= 4)
		lapicw(PCINT, MASKED);

	// Map error interrupt to IRQ_ERROR.
	lapicw(ERROR, T_IRQ0 + IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	lapicw(ESR, 0);
	lapicw(ESR, 0);

	// Ack any outstanding interrupts.
	lapicw(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	lapicw(ICRHI, 0);
	lapicw(ICRLO, BCAST | INIT | LEVEL);
	while (lapic[ICRLO] & DELIVS)
		;

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(TPR, 0);
	initlock(&rtc_lock, "rtc");
}

int
lapicid(void)
{
	if (!lapic)
		return 0;
	return lapic[ID] >> 24;
}

// Acknowledge interrupt.
void
lapiceoi(void)
{
	if (lapic)
		lapicw(EOI, 0);
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
	for (int i = 0; i < us; i++)
		inb(0x80);
}

#define CMOS_PORT 0x70
#define CMOS_RETURN 0x71

// Start additional processor running entry code at addr.
// See Appendix B of MultiProcessor Specification.
__suppress_sanitizer("alignment") void lapicstartap(uint8_t apicid,
																										uint32_t addr)
{
	int i;
	uint16_t *wrv;

	// "The BSP must initialize CMOS shutdown code to 0AH
	// and the warm reset vector (DWORD based at 40:67) to point at
	// the AP startup code prior to the [universal startup algorithm]."
	outb(CMOS_PORT, 0xF); // offset 0xF is shutdown code
	outb(CMOS_PORT + 1, 0x0A);
	wrv = (uint16_t *)P2V((0x40 << 4 | 0x67)); // Warm reset vector
	wrv[0] = 0;
	wrv[1] = addr >> 4;

	// "Universal startup algorithm."
	// Send INIT (level-triggered) interrupt to reset other CPU.
	lapicw(ICRHI, apicid << 24);
	lapicw(ICRLO, INIT | LEVEL | ASSERT);
	microdelay(200);
	lapicw(ICRLO, INIT | LEVEL);
	microdelay(100); // should be 10ms, but too slow in Bochs!

	// Send startup IPI (twice!) to enter code.
	// Regular hardware is supposed to only accept a STARTUP
	// when it is in the halted state due to an INIT.  So the second
	// should be ignored, but it is part of the official Intel algorithm.
	// Bochs complains about the second one.  Too bad for Bochs.
	for (i = 0; i < 2; i++) {
		lapicw(ICRHI, apicid << 24);
		lapicw(ICRLO, STARTUP | (addr >> 12));
		microdelay(200);
	}
}

#define CMOS_STATA 0x0a
#define CMOS_STATB 0x0b
#define CMOS_UIP (1 << 7) // RTC update in progress

// these are "registers"
#define SECS 0x00
#define MINS 0x02
#define HOURS 0x04 // 24hr or 12; highest bit set if pm
#define DAY_OF_WEEK 0x06 // 1 = sunday
#define DAY 0x07 // 1-31
#define MONTH 0x08 // 1-12
#define YEAR 0x09 // 0-99
// untrustworthy; the real century register is in ACPI FADT offset 108
#define CENTURY 0x32

static uint8_t
cmos_read(uint8_t reg)
{
	outb(CMOS_PORT, reg);
	microdelay(200);

	return inb(CMOS_RETURN);
}

static void
fill_rtcdate(struct rtcdate *r)
{
	r->second = cmos_read(SECS);
	r->minute = cmos_read(MINS);
	r->hour = cmos_read(HOURS);
	r->day = cmos_read(DAY);
	r->month = cmos_read(MONTH);
	r->year = cmos_read(YEAR);
}
static uint8_t
bcd_to_binary(uint8_t bcd)
{
	return ((bcd >> 4) * 10) + (bcd & 0xf);
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
void
cmostime(struct rtcdate *r)
{
	struct rtcdate t1;
	int sb, bcd;

	sb = cmos_read(CMOS_STATB);

	// find out if the cmos is bcd-encoded
	bcd = (sb & (1 << 2)) == 0;

	// make sure CMOS doesn't modify time while we read it
	size_t time_passed_in_ms = 0;
	bool update_in_progress_ended_successfully = false;
	while (time_passed_in_ms < 100) {
		if (!(cmos_read(CMOS_STATA) & CMOS_UIP)) {
			update_in_progress_ended_successfully = true;
			break;
		}
		microdelay(1000);
		time_passed_in_ms++;
	}
	acquire(&rtc_lock);
	if (!update_in_progress_ended_successfully) {
		t1.year = 1970;
		t1.month = 1;
		t1.day = 1;
		t1.hour = 0;
		t1.minute = 0;
		t1.second = 0;
		release(&rtc_lock);
		return;
	}
	fill_rtcdate(&t1);

	bool is_pm = t1.hour & 0x80;
	// convert
	if (bcd) {
		t1.second = bcd_to_binary(t1.second);
		t1.minute = bcd_to_binary(t1.minute);
		t1.hour = bcd_to_binary(t1.hour & 0x7F);
		t1.day = bcd_to_binary(t1.day);
		t1.month = bcd_to_binary(t1.month);
		t1.year = bcd_to_binary(t1.year);
	}
	if ((sb & (1 << 1)) == 0) {
		t1.hour %= 12;
		if (is_pm)
			t1.hour += 12;
	}
	t1.year += 2000;
	release(&rtc_lock);

	*r = t1;
}
