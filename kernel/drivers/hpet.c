/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "hpet.h"
#include "acpi.h"
#include "console.h"
#include "cpu_units.h"
#include "ioapic.h"
#include "kernel_assert.h"
#include "memlayout.h"
#include "time_units.h"
#include "traps.h"
#include "x86.h"
#include <stdbool.h>
#include <stdint.h>

static volatile struct hpet_registers *s_hpet_regs;
static uint32_t s_period_fs;

#define GENCAP_REV_ID(cap) (uint8_t)((cap) & 0xff)
#define GENCAP_NUM_TIM_CAP(cap) (uint8_t)(((cap) >> 8 & 0xf) + 1)
#define GENCAP_COUNT_SIZE_CAP(cap) (bool)((cap) >> 13 & 0b1)
#define GENCAP_LEG_ROUTE_CAP(cap) (bool)((cap) >> 15 & 0b1)
#define GENCAP_VENDOR_ID(cap) (uint16_t)((cap) >> 16 & 0xffff)
#define GENCAP_COUNTER_CLK_PERIOD(cap) (uint32_t)((cap) >> 32)

#define GENCONF_ENABLE_CNF(conf) (bool)((conf) & 0b1)
#define GENCONF_LEG_RT_CNF(conf) (bool)((conf) >> 1 & 0b1)

#define Tn_INT_TYPE_CNF(tc) (bool)((tc) >> 1 & 0b1)
#define Tn_INT_ENB_CNF(tc) (bool)((tc) >> 2 & 0b1)
#define Tn_TYPE_CNF(tc) (bool)((tc) >> 3 & 0b1)
#define Tn_PER_INT_CAP(tc) (bool)((tc) >> 4 & 0b1)
#define Tn_SIZE_CAP(tc) (bool)((tc) >> 5 & 0b1)
#define Tn_VAL_SET_CNF(tc) (bool)((tc) >> 6 & 0b1)
#define Tn_32MODE_CNF(tc) (bool)((tc) >> 8 & 0b1)
#define Tn_INT_ROUTE_CNF(tc) (uint8_t)((tc) >> 9 & 0b11111)
#define Tn_FSB_EN_CNF(tc) (bool)((tc) >> 14 & 0b1)
#define Tn_FSB_INT_DEL_CAP(tc) (bool)((tc) >> 15 & 0b1)
#define Tn_INT_ROUTE_CAP(tc) (uint32_t)((tc) >> 32 & 0xffffffff)
#define Tn_INT_ROUTE_CAP_N(tc, i) (uint32_t)((tc) >> (32 + (i)) & 0xffffffff)

volatile struct hpet_registers *
hpet_regs_get(void)
{
	return s_hpet_regs;
}

uint64_t
hpet_get_counter_period_fs(void)
{
	return s_period_fs;
}

/*
 * Below is an example on how to use these APIs:
 * hpet_timer_set_ns(hpet_get_timer_n(0), NSEC_PER_SEC);
 */
void
hpet_timer_set_ns(volatile struct hpet_timer *timer, __time_t time_ns)
{
	if (time_ns < s_period_fs) {
		time_ns = s_period_fs;
	}
	__time_t time_in_comparator_ticks = time_ns / fsec_to_nsec(s_period_fs);

	timer->comparator_value =
		s_hpet_regs->main_counter_value + time_in_comparator_ticks;
	timer->config_and_capabilities |= (1 << 2); // enable
}

static void
hpet_timer_clear(volatile struct hpet_timer *timer)
{
	timer->config_and_capabilities &= ~(1 << 2); // disable
}

static void
hpet_timer_set_us(volatile struct hpet_timer *timer, __time_t time_us)
{
	hpet_timer_set_ns(timer, usec_to_nsec(time_us));
}

static void
hpet_timer_set_ms(volatile struct hpet_timer *timer, __time_t time_ms)
{
	hpet_timer_set_us(timer, msec_to_usec(time_ms));
}

static void
hpet_timer_set(volatile struct hpet_timer *timer, __time_t seconds)
{
	hpet_timer_set_ms(timer, sec_to_msec(seconds));
}

static void
hpet_enable(void)
{
	s_hpet_regs->general_configuration |= (1 << 0);
}

static void
hpet_disable(void)
{
	s_hpet_regs->general_configuration &= ~(1 << 0);
}

static void
hpet_set_legacy_replacement(void)
{
	uint64_t caps = s_hpet_regs->general_capabilities;
	if (!GENCAP_LEG_ROUTE_CAP(caps)) {
		pr_debug_file("tried to set legacy replacement when we don't support it\n");
		return;
	}

	s_hpet_regs->general_configuration |= (1 << 1);
}

volatile struct hpet_timer *
hpet_get_timer_n(uint8_t n)
{
	kernel_assert(n < 32);
	return &s_hpet_regs->timers[n];
}

void
hpet_init(struct acpi_hpet *hpet)
{
	volatile struct hpet_registers *regs = IO2V(hpet->address.address);
	s_hpet_regs = regs;
	// General capabilities discovery.
	uint64_t capabilities = regs->general_capabilities;
	uint8_t rev_id = GENCAP_REV_ID(capabilities);
	uint8_t num_time_cap = GENCAP_NUM_TIM_CAP(capabilities);
	bool count_size_cap = GENCAP_COUNT_SIZE_CAP(capabilities);
	bool can_do_legacy_replacement_mapping = GENCAP_LEG_ROUTE_CAP(capabilities);
	uint16_t vendor_id = GENCAP_VENDOR_ID(capabilities);
	uint32_t counter_clock_period_fs = GENCAP_COUNTER_CLK_PERIOD(capabilities);
	s_period_fs = counter_clock_period_fs;

	// General configuration discovery.
	uint64_t general_config = regs->general_configuration;
	bool enable_cnf = GENCONF_ENABLE_CNF(general_config);
	bool leg_rt_cnf = GENCONF_LEG_RT_CNF(general_config);

	uint64_t ticks = FSEC_PER_SEC / counter_clock_period_fs;
	uart_printf(
		"hpet: vendor %#x v%d, %d-bit, %d timers, period=%luns (%lu.%03luMHz)\n",
		vendor_id, rev_id, count_size_cap ? 64 : 32, num_time_cap,
		fsec_to_nsec(counter_clock_period_fs), ticks / MHz, ticks % MHz);
	hpet_set_legacy_replacement();

	for (int i = 0; i < num_time_cap; i++) {
		struct hpet_timer timer = regs->timers[i];

		uint64_t caps = timer.config_and_capabilities;
		uart_printf("periodic_capable=%d %dbit\n", Tn_PER_INT_CAP(caps),
		            Tn_SIZE_CAP(caps) ? 64 : 32);

		// caps |= (1 << 1); // leveled, not edge
		caps &= ~(1 << 1); // edge, not leveled
		caps &= ~(1 << 2); // disable
		// If periodic is supported, disable it.
		// The Intel spec says that writes have no impact when periodic mode
		// is not supported, so we can unconditionally clear this bit.
		caps &= ~(1 << 3); // oneshot, not periodic
		caps &= ~(1 << 8); // disable forced 32 bit mode
		caps &= ~(0b11111 << 9); // clear ioapic routing
		caps &= ~(1 << 14); // disable FSB interrupts

		uart_printf("%s-triggered enabled=%d %s accumulator_setting=%s\n",
		            Tn_INT_TYPE_CNF(caps) ? "level" : "edge", Tn_INT_ENB_CNF(caps),
		            Tn_TYPE_CNF(caps) ? "periodic" : "oneshot",
		            Tn_VAL_SET_CNF(caps) ? "yes" : "no");

		uart_printf("Can map to one of these: ");
		for (int j = 0; j < 32; j++) {
			if (Tn_INT_ROUTE_CAP_N(caps, j)) {
				uart_printf("IRQ%d ", j);
			}
		}
		uart_printf("\n");
	}
	if ((regs->timers[0].config_and_capabilities >> 32) & (1 << 2)) {
		uart_printf("Timer 0 irq 2: enabling\n");
		regs->timers[0].config_and_capabilities |= 2 << 9; // ioapic routing = 2
		uart_printf("enabled=%d, ioapic routing: %d\n",
		            Tn_INT_ENB_CNF(regs->timers[0].config_and_capabilities),
		            Tn_INT_ROUTE_CNF(regs->timers[0].config_and_capabilities));
	}
	hpet_enable();

	ioapicenable(IRQ_HPET, 0);
}
