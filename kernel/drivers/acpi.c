/*
 *
 * Copyright (c) 2013 Brian Swetland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stddef.h>
#include <stdint.h>
#include "stdint.h"
#include "param.h"
#include "memlayout.h"
#include "proc.h"
#include "acpi.h"
#include "kernel_string.h"
#include "kernel_assert.h"
#include "console.h"
#include "lapic.h"

extern struct cpu cpus[NCPU];
extern int ncpu;
extern uint8_t ioapicid;

static int
do_checksum(struct acpi_desc_header *dsc)
{
	uint32_t sum = 0;
	for (int i = 0; i < dsc->length; i++) {
		sum += ((char *)dsc)[i];
	}
	return (sum & 0xff) == 0;
}
static int
do_checksum_rsdp(struct acpi_rsdp *r, uint32_t len)
{
	uint32_t sum = 0;
	for (int i = 0; i < len; i++) {
		sum += ((char *)r)[i];
	}
	return (sum & 0xff) == 0;
}

static struct acpi_rsdp *
scan_rsdp(uint32_t base, uint32_t len)
{
	uint8_t *p;

	for (p = p2v(base); len >= sizeof(struct acpi_rsdp);
			 len -= sizeof(len), p += sizeof(p)) {
		if (memcmp(p, SIG_RSDP, 8) == 0 &&
				do_checksum_rsdp((struct acpi_rsdp *)p, 20)) {
			return (struct acpi_rsdp *)p;
		}
	}
	return (struct acpi_rsdp *)0;
}
#define acpi_cprintf(...) cprintf("acpi: " __VA_ARGS__)
static void
dump_rsdp(struct acpi_rsdp *rsdp)
{
	char rsdp_oem_id[7];
	strlcpy_nostrlen(rsdp_oem_id, (char *)rsdp->oem_id, sizeof(rsdp_oem_id),
									 sizeof(rsdp->oem_id));

	acpi_cprintf("oem id: %s\n", rsdp_oem_id);
	acpi_cprintf("revision: %d\n", rsdp->revision + 1);
}

// the rsdp can either be in the EBDA area
// (found from a pointer in P0x40E and a length at P0x413)
// or it can be found in a memory region from 0xE0000-0xFFFFF.
// returns the PHYSICAL address.
static struct acpi_rsdp *
find_rsdp(void)
{
	struct acpi_rsdp *rsdp;
	uintptr_t pa;
	// PA is mapped in kmap; vm.c. TODO: mappages needs to be dynamic
	pa = *((uint16_t *)(p2v(0x40 << 4 | 0x0E))); // EBDA pointer
	rsdp = scan_rsdp(pa, 1 * kiB);
	if (pa && (rsdp != NULL))
		return rsdp;
	acpi_cprintf("rsdp not in EDBA; trying BIOS memory.\n");
	return scan_rsdp(0xE0000, 0x20000);
}

static int
acpi_config_smp(struct acpi_madt *madt)
{
	uint32_t lapic_addr;
	uint32_t nioapic = 0;
	uint8_t *p, *e;

	if (!madt)
		return -1;
	if (madt->header.length < sizeof(struct acpi_madt))
		return -1;

	lapic_addr = madt->lapic_addr_phys;

	p = madt->table;
	e = p + madt->header.length - sizeof(struct acpi_madt);

	while (p < e) {
		uint32_t len;
		if ((e - p) < 2)
			break;
		len = p[1];
		if ((e - p) < len)
			break;
		switch (p[0]) {
		case TYPE_LAPIC: {
			struct madt_lapic *lapic = (void *)p;
			if (len < sizeof(*lapic))
				break;
			if (!(lapic->flags & APIC_LAPIC_ENABLED))
				break;
			acpi_cprintf("cpu#%d apicid %d\n", ncpu, lapic->apic_id);
			cpus[ncpu].apicid = lapic->apic_id;
			ncpu++;
			break;
		}
		case TYPE_IOAPIC: {
			struct madt_ioapic *ioapic = (void *)p;
			if (len < sizeof(*ioapic))
				break;
			acpi_cprintf("ioapic#%d @%x id=%d base=%d\n", nioapic, ioapic->addr,
									 ioapic->id, ioapic->interrupt_base);
			if (nioapic) {
				acpi_cprintf("warning: multiple ioapics are not supported");
			} else {
				ioapicid = ioapic->id;
			}
			nioapic++;
			break;
		}
		}
		p += len;
	}

	if (ncpu) {
		lapic = IO2V(((uintptr_t)lapic_addr));
		return 0;
	}

	return -1;
}

static void
setup_fadt(struct acpi_fadt *fadt)
{
	if (!fadt)
		return;
	acpi_cprintf("Century: %d\n", fadt->century);
}

static int
try_setup_headers_xsdt(struct acpi_xsdt *xsdt)
{
	struct acpi_madt *madt = NULL;
	struct acpi_fadt *fadt;
	int count = (xsdt->header.length - sizeof(struct acpi_desc_header)) /
							sizeof(*xsdt->entry);
	for (int n = 0; n < count; n++) {
		struct acpi_desc_header *hdr = p2v(xsdt->entry[n]);
		if (xsdt->entry[n] > PHYSLIMIT)
			goto notmapped;
		uint8_t sig[5], id[7], tableid[9], creator[5];
		memmove(sig, hdr->signature, 4);
		sig[4] = '\0';
		memmove(id, hdr->oem_id, 6);
		id[6] = '\0';
		memmove(tableid, hdr->oem_tableid, 8);
		tableid[8] = '\0';
		memmove(creator, hdr->creator_id, 4);
		creator[4] = '\0';
		acpi_cprintf("%s %s %s %x %s %x\n", sig, id, tableid, hdr->oem_revision,
								 creator, hdr->creator_revision);

		if (memcmp(hdr->signature, SIG_MADT, 4) == 0)
			madt = (void *)hdr;
		else if (memcmp(hdr->signature, SIG_FADT, 4) == 0) {
			fadt = (void *)hdr;
			setup_fadt(fadt);
		}
	}
	kernel_assert(madt != NULL);
	return acpi_config_smp(madt);

notmapped:
	acpi_cprintf("rsdt entry pointer above %#lx\n", PHYSLIMIT);
	return -1;
}

static int
try_setup_headers_rsdt(struct acpi_rsdt *rsdt)
{
	struct acpi_madt *madt = NULL;
	struct acpi_fadt *fadt;
	int count = (rsdt->header.length - sizeof(struct acpi_desc_header)) /
							sizeof(*rsdt->entry);
	for (int n = 0; n < count; n++) {
		struct acpi_desc_header *hdr = p2v(rsdt->entry[n]);
		if (rsdt->entry[n] > PHYSLIMIT)
			goto notmapped;
		uint8_t sig[5], id[7], tableid[9], creator[5];
		memmove(sig, hdr->signature, 4);
		sig[4] = '\0';
		memmove(id, hdr->oem_id, 6);
		id[6] = '\0';
		memmove(tableid, hdr->oem_tableid, 8);
		tableid[8] = '\0';
		memmove(creator, hdr->creator_id, 4);
		creator[4] = '\0';
		acpi_cprintf("%s %s %s %x %s %x\n", sig, id, tableid, hdr->oem_revision,
								 creator, hdr->creator_revision);

		if (memcmp(hdr->signature, SIG_MADT, 4) == 0)
			madt = (void *)hdr;
		else if (memcmp(hdr->signature, SIG_FADT, 4) == 0) {
			fadt = (void *)hdr;
			setup_fadt(fadt);
		}
	}
	kernel_assert(madt != NULL);
	return acpi_config_smp(madt);

notmapped:
	acpi_cprintf("rsdt entry pointer above %#lx\n", PHYSLIMIT);
	return -1;
}

int
acpiinit(void)
{
	struct acpi_rsdp *rsdp;
	struct acpi_rsdt *rsdt;
	struct acpi_xsdt *xsdt;

	rsdp = find_rsdp();
	if (rsdp == NULL)
		panic("NULL rsdp");
	dump_rsdp(rsdp);
	if (rsdp->revision + 1 == 1 && rsdp->rsdt_addr_phys > PHYSLIMIT)
		goto notmapped_rsdt;
	else if (rsdp->revision + 1 > 1 && rsdp->xsdt_addr_phys > PHYSLIMIT)
		goto notmapped_xsdt;
	if (rsdp->revision + 1 > 1) {
		xsdt = p2v(rsdp->xsdt_addr_phys);
		acpi_cprintf("xsdt physical address: P%#lx\n", rsdp->xsdt_addr_phys);
		kernel_assert(do_checksum(&xsdt->header) == 1);
		kernel_assert(memcmp(xsdt->header.signature, "XSDT", 4) == 0);
		return try_setup_headers_xsdt(xsdt);
	} else {
		rsdt = p2v(rsdp->rsdt_addr_phys);
		acpi_cprintf("rsdt physical address: P%#x\n", rsdp->rsdt_addr_phys);
		kernel_assert(do_checksum(&rsdt->header) == 1);
		kernel_assert(memcmp(rsdt->header.signature, "RSDT", 4) == 0);
		return try_setup_headers_rsdt(rsdt);
	}

notmapped_rsdt:
	acpi_cprintf("rsdt_addr_phs %#x > %#lx\n", rsdp->rsdt_addr_phys, PHYSLIMIT);
	return -1;
notmapped_xsdt:
	acpi_cprintf("xsdt_addr_phs %#lx > %#lx\n", rsdp->xsdt_addr_phys, PHYSLIMIT);
	return -1;
}
