// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include "lapic.h"
#include "kernel_assert.h"
#include "lib/compiler_attributes.h"
#include "memlayout.h"
#include "spinlock.h"
#include "stdbool.h"
#include "traps.h"
#include "x86.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

volatile uint32_t *lapic; // Initialized in mp.c

struct spinlock rtc_lock;

void
lapicw(int index, int value)
{
	lapic[index] = value;
	lapic[ID]; // wait for write to finish, by reading
}

void
lapicinit(void)
{
	if (!lapic) {
		return;
	}

	// Enable local APIC; set spurious interrupt vector.
	lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

	// The timer repeatedly counts down at bus frequency
	// from lapic[TICR] and then issues an interrupt.
	// If relix cared more about precise timekeeping,
	// TICR would be calibrated using an external time source.
	uint32_t eax, ebx, ecx, edx;
	cpuid(6, 0, &eax, &ebx, &ecx, &edx);
	// considered the "ARAT" bit
	// Intel manual 12.5.4
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
	if (((lapic[VER] >> 16) & 0xFF) >= 4) {
		lapicw(PCINT, MASKED);
	}

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
	if (!lapic) {
		return 0;
	}
	return lapic[ID] >> 24;
}

// Acknowledge interrupt.
void
lapiceoi(void)
{
	if (lapic) {
		lapicw(EOI, 0);
	}
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
void
microdelay(int us)
{
	for (int i = 0; i < us; i++) {
		inb(0x80);
	}
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
	return inb(CMOS_RETURN);
}

static uint8_t
bcd_to_binary(uint8_t bcd)
{
	return ((bcd >> 4) * 10) + (bcd & 0xf);
}

static inline bool
is_leap_year(unsigned year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static inline unsigned
days_in_year(unsigned year)
{
	return is_leap_year(year) ? 366 : 365;
}

// `month`: January is 1

static inline unsigned
days_in_month(unsigned year, unsigned month)
{
	static const unsigned table[] = { 31, 28, 31, 30, 31, 30,
		                                31, 31, 30, 31, 30, 31 };
	if (month == 2 && is_leap_year(year)) {
		return 29;
	}
	return table[month - 1];
}

static inline unsigned
day_of_year(unsigned year, unsigned month, unsigned day)
{
	unsigned result = day - 1;
	for (unsigned m = 1; m < month; ++m) {
		result += days_in_month(year, m);
	}
	return result;
}
static unsigned
days_since_epoch(unsigned year, unsigned month, unsigned day)
{
	kernel_assert(year >= 1970);
	unsigned days = day_of_year(year, month, day);
	for (unsigned y = 1970; y < year; ++y) {
		days += days_in_year(y);
	}
	return days;
}

// qemu seems to use 24-hour GWT and the values are BCD encoded
time_t
rtc_now(void)
{
	int sb, bcd;

	sb = cmos_read(CMOS_STATB);
	time_t year, month, day, hour, minute, second;

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
	if (update_in_progress_ended_successfully) {
		second = cmos_read(SECS);
		minute = cmos_read(MINS);
		hour = cmos_read(HOURS);
		day = cmos_read(DAY);
		month = cmos_read(MONTH);
		year = cmos_read(YEAR);

		bool is_pm = hour & 0x80;
		// convert
		if (bcd) {
			second = bcd_to_binary(second);
			minute = bcd_to_binary(minute);
			hour = bcd_to_binary(hour & 0x7F);
			day = bcd_to_binary(day);
			month = bcd_to_binary(month);
			year = bcd_to_binary(year);
		}
		if ((sb & (1 << 1)) == 0) {
			hour %= 12;
			if (is_pm) {
				hour += 12;
			}
		}
		year += 2000;
	} else {
		year = 1970;
		month = 1;
		day = 1;
		hour = 0;
		minute = 0;
		second = 0;
	}
	time_t days = days_since_epoch(year, month, day);
	time_t hours = days * 24 + hour;
	time_t minutes = hours * 60 + minute;
	return minutes * 60 + second;
}
