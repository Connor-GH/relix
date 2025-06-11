// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "mp.h"
#include "console.h"
#include "drivers/lapic.h"
#include "drivers/memlayout.h"
#include "kernel_assert.h"
#include "lib/compiler_attributes.h"
#include "param.h"
#include "proc.h"
#include "x86.h"
#include <stdint.h>
#include <string.h>

struct cpu cpus[NCPU];
int ncpu;
uint8_t ioapicid;

static uint8_t
checksum(uint8_t *addr, int len)
{
	int sum = 0;

	for (int i = 0; i < len; i++) {
		sum += addr[i];
	}
	return sum;
}

// Look for an MP structure in the len bytes at addr.
static struct mp *
mpsearch1(uint32_t a, int len)
{
	uint8_t *addr = p2v(a);
	uint8_t *e = addr + len;

	for (uint8_t *p = addr; p < e; p += sizeof(struct mp)) {
		// see if we found the _MP_ signature that we need.
		// https://web.archive.org/web/20121002210153/http://download.intel.com/design/archives/processors/pro/docs/24201606.pdf
		if (memcmp(p, "_MP_", 4) == 0 && checksum(p, sizeof(struct mp)) == 0) {
			return (struct mp *)p;
		}
	}
	return NULL;
}

// Search for the MP Floating Pointer Structure, which according to the
// spec is in one of the following three locations:
// 1) in the first KB of the EBDA;
// 2) in the last KB of system base memory;
// 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
static struct mp *
mpsearch(void)
{
	uint32_t p;
	struct mp *mp;

	uint8_t *bda = (uint8_t *)P2V(0x400);

	if ((p = ((bda[0x0F] << 8) | bda[0x0E]) << 4)) {
		if ((mp = mpsearch1(p, 1024))) {
			return mp;
		}
	} else {
		p = ((bda[0x14] << 8) | bda[0x13]) * 1024;
		if ((mp = mpsearch1(p - 1024, 1024))) {
			return mp;
		}
	}
	return mpsearch1(0xF0000, 0x10000);
}

// Search for an MP configuration table.  For now,
// don't accept the default configurations (physaddr == 0).
// Check for correct signature, calculate the checksum and,
// if correct, check the version.
// To do: check extended table checksum.
__nonnull(1) static struct mpconf *mpconfig(struct mp **pmp)
{
	struct mpconf *conf;
	struct mp *mp;

	if ((mp = mpsearch()) == NULL || mp->physaddr == NULL) {
		return NULL;
	}
	conf = (struct mpconf *)p2v((uintptr_t)mp->physaddr);
	if (memcmp(conf, "PCMP", 4) != 0) {
		return NULL;
	}
	if (conf->version != 1 && conf->version != 4) {
		return NULL;
	}
	if (checksum((uint8_t *)conf, conf->length) != 0) {
		return NULL;
	}
	*pmp = mp;
	return conf;
}

void
mpinit(void)
{
	uint8_t *p, *e;
	int ismp;
	struct mp *mp;
	struct mpconf *conf;
	struct mpproc *proc;
	struct mpioapic *ioapic;
	struct mpbus *bus;

	if ((conf = mpconfig(&mp)) == NULL) {
		panic("Expect to run on an SMP");
	}
	ismp = 1;
	lapic = IO2V((uintptr_t)conf->lapicaddr);
	for (p = (uint8_t *)(conf + 1), e = (uint8_t *)conf + conf->length; p < e;) {
		switch (*p) {
		case MPPROC:
			proc = (struct mpproc *)p;
			uint32_t a, b, c, d;
			uint32_t family, model;
			cpuid(1, 0, &a, &b, &c, &d);
			proc->signature[0] = a & 0xF; // stepping
			proc->signature[1] = (a >> 4) & 0xF; // model
			proc->signature[2] = (a >> 8) & 0xF; // family
			proc->signature[3] = (a >> 20) & 0xFF; // extended family
			kernel_assert(a != 0);
			family = proc->signature[2] + proc->signature[3];
			model = proc->signature[1];
			model += ((a >> 16) & 0xF) << 4;
			cprintf("CPU Model=%#x, Family=%#x Stepping=%#x\n", model, family,
			        (char)proc->signature[0] /*stepping*/);
			if (ncpu < NCPU) {
				cpus[ncpu].apicid = proc->apicid; // apicid may differ from ncpu
				ncpu++;
			}
			p += sizeof(struct mpproc);
			continue;
		case MPIOAPIC:
			ioapic = (struct mpioapic *)p;
			ioapicid = ioapic->apicno;
			p += sizeof(struct mpioapic);
			continue;
		case MPBUS:
			bus = (struct mpbus *)p;
			char bus_str[7];
			__safestrcpy(bus_str, (char *)bus->bus_string, 6);
			cprintf("Bus discovered: %s\n", bus_str);
			p += sizeof(struct mpbus);
			continue;
		case MPIOINTR:
		case MPLINTR:
			p += 8;
			continue;
		default:
			ismp = 0;
			break;
		}
	}
	if (!ismp) {
		panic("Didn't find a suitable machine");
	}

	if (mp->imcrp) {
		// Bochs doesn't support IMCR, so this doesn't run on Bochs.
		// But it would on real hardware.
		outb(0x22, 0x70); // Select IMCR
		outb(0x23, inb(0x23) | 1); // Mask external interrupts.
	}
}
