#pragma once
#include "bits/types.h"
#include <stddef.h>
#include <stdint.h>

struct hpet_timer {
	//    +-------------------------------------------------------------------+
	//  0 | Rsvd0 | Edge/Level | Enabled? | P/OS | PC? | 64bit | CDSA | Rsvd1 |  7
	//    +-------------------------------------------------------------------+
	//  8 | Force 32b | IOAPIC Routing (5 bits) | FSB Interrupt | FSB Support | 15
	//    +-------------------------------------------------------------------+
	// 16 | Reserved2 (16 bits)                                               | 31
	//    +-------------------------------------------------------------------+
	// 32 | Routing Capability (32 bits)                                      | 63
	//    +-------------------------------------------------------------------+
	// CDSA = can directly set accumulator
	// PC = periodic capable
	// P/OS = periodic or oneshot
	volatile uint64_t config_and_capabilities;
	// Used to check if an interrupt should be generated.
	volatile uint64_t comparator_value;
	volatile uint64_t fsb_interrupt_route;
	volatile uint64_t unused;
} __attribute__((__packed__));

struct hpet_registers {
	//   63:32   31:16   15   14   13   12:8   7:0
	// [       |       |    |  R |    |      |     ]
	//     ^       ^     ^          ^     ^     ^- revision id
	//     |       |     |          |     |- number of timers
	//     |       |     |          |-counter size
	//     |       |     |- legacy replacement
	//     |       |- vendor id
	//     |-counter tick period (femtoseconds)
	volatile uint64_t general_capabilities;
	volatile uint64_t unused0;
	volatile uint64_t general_configuration;
	volatile uint64_t unused1;
	volatile uint64_t general_int_status;
	volatile uint64_t unused2;
	volatile uint64_t unused3[24];
	volatile uint64_t main_counter_value;
	volatile uint64_t unused4;
	volatile struct hpet_timer timers[];
} __attribute__((__packed__));
_Static_assert(offsetof(struct hpet_registers, general_capabilities) == 0x0,
               "");
_Static_assert(offsetof(struct hpet_registers, general_configuration) == 0x10,
               "");
_Static_assert(offsetof(struct hpet_registers, general_int_status) == 0x20, "");
_Static_assert(offsetof(struct hpet_registers, main_counter_value) == 0x0F0,
               "");
// You can use induction to prove that the offset for these timers are correct.
// A proof solver would be able to test timers[N] for an arbitrary N.
_Static_assert(offsetof(struct hpet_registers, timers[0]) == 0x100 + 0x20 * 0,
               "");
_Static_assert(offsetof(struct hpet_registers, timers[1]) == 0x100 + 0x20 * 1,
               "");
struct acpi_hpet;
void hpet_init(struct acpi_hpet *hpet);
volatile struct hpet_registers *hpet_get_regs(void);
uint64_t hpet_get_counter_period_fs(void);
volatile struct hpet_timer *hpet_get_timer_n(uint8_t n);
void hpet_timer_set_ns(volatile struct hpet_timer *timer, __time_t time_ns);
void hpet_decrease_counter_ticks_by_period(void);

extern struct spinlock hpet_lock;
extern bool hpet_waiting;
