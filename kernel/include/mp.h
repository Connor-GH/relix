#pragma once
#include <types.h>
// See MultiProcessor Specification Version 1.[14]

extern int ismp; // SMP

struct mp { // floating pointer
	uchar signature[4]; // "_MP_"
	void *physaddr; // phys addr of MP config table
	uchar length; // 1
	uchar specrev; // [14]
	uchar checksum; // all bytes must add up to 0
	uchar type; // MP system config type. when zero, the MP table is present.
	uchar imcrp; // [0-6] reserved; 7 is imcrp.
	uchar reserved[3]; // must be zero.
};

struct mpconf { // configuration table header
	uchar signature[4]; // "PCMP"
	ushort length; // total table length
	uchar version; // [14]; actually just the revision
	uchar checksum; // all bytes must add up to 0
	uchar
		product[20]; // product id; first 4 is opem string; last 16 is product string
	uint *oemtable; // OEM table pointer
	ushort oemlength; // OEM table length
	ushort entry; // entry count
	uint *lapicaddr; // address of local APIC
	ushort xlength; // extended table length
	uchar xchecksum; // extended table checksum
	uchar reserved;
};

struct mpproc { // processor table entry
	uchar type; // entry type (0)
	uchar apicid; // local APIC id
	uchar version; // local APIC version
	uchar flags; // CPU flags; EN, BP, [2-7] reserved.
#define MPBOOT 0x02 // This proc is the bootstrap processor.
	uchar signature[4]; // CPU signature
	uint feature; // feature flags from CPUID instruction
	uchar reserved[8];
};

struct mpioapic { // I/O APIC table entry
	uchar type; // entry type (2)
	uchar apicno; // I/O APIC id
	uchar version; // I/O APIC version
	uchar flags; // I/O APIC flags; only bit 0 is defined.
	uint *addr; // I/O APIC address
};

struct mpbus {
	uchar type;
	uchar busid;
	uchar bus_string[6];
};

// Table entry types
#define MPPROC 0x00 // One per processor
#define MPBUS 0x01 // One per bus
#define MPIOAPIC 0x02 // One per I/O APIC
#define MPIOINTR 0x03 // One per bus interrupt source
#define MPLINTR 0x04 // One per system interrupt source

extern int ismp;
void
mpinit(void);
