#pragma once
#include <stdint.h>
#include <stdint.h>
#include "compiler_attributes.h"
// Routines to let C code use special x86 instructions.

static __always_inline uint8_t
inb(uint16_t port)
{
	uint8_t data;

	__asm__ __volatile__("in %1,%0" : "=a"(data) : "d"(port));
	return data;
}

static __always_inline void
insl(int port, void *addr, int cnt)
{
	__asm__ __volatile__("cld; rep insl"
											 : "=D"(addr), "=c"(cnt)
											 : "d"(port), "0"(addr), "1"(cnt)
											 : "memory", "cc");
}

static __always_inline void
outb(uint16_t port, uint8_t data)
{
	__asm__ __volatile__("out %0,%1" : : "a"(data), "d"(port));
}

static __always_inline void
outw(uint16_t port, uint16_t data)
{
	__asm__ __volatile__("out %0,%1" : : "a"(data), "d"(port));
}

static __always_inline void
outsl(int port, const void *addr, int cnt)
{
	__asm__ __volatile__("cld; rep outsl"
											 : "=S"(addr), "=c"(cnt)
											 : "d"(port), "0"(addr), "1"(cnt)
											 : "cc");
}

static __always_inline void
stosb(void *addr, int data, int cnt)
{
	__asm__ __volatile__("cld; rep stosb"
											 : "=D"(addr), "=c"(cnt)
											 : "0"(addr), "1"(cnt), "a"(data)
											 : "memory", "cc");
}

static __always_inline void
stosl(void *addr, int data, int cnt)
{
	__asm__ __volatile__("cld; rep stosl"
											 : "=D"(addr), "=c"(cnt)
											 : "0"(addr), "1"(cnt), "a"(data)
											 : "memory", "cc");
}

struct segdesc;

static __always_inline void
lgdt(struct segdesc *p, int size)
{
	volatile uint16_t pd[3];

	pd[0] = size - 1;
	pd[1] = (uint32_t)p;
	pd[2] = (uint32_t)p >> 16;

	__asm__ __volatile__("lgdt (%0)" : : "r"(pd));
}

struct gatedesc;

static __always_inline void
lidt(struct gatedesc *p, int size)
{
	volatile uint16_t pd[3];

	pd[0] = size - 1;
	pd[1] = (uint32_t)p;
	pd[2] = (uint32_t)p >> 16;

	__asm__ __volatile__("lidt (%0)" : : "r"(pd));
}

static __always_inline void
ltr(uint16_t sel)
{
	__asm__ __volatile__("ltr %0" : : "r"(sel));
}

static __always_inline uint32_t
readeflags(void)
{
	uint32_t eflags;
	__asm__ __volatile__("pushfl; popl %0" : "=r"(eflags));
	return eflags;
}

static __always_inline void
loadgs(uint16_t v)
{
	__asm__ __volatile__("movw %0, %%gs" : : "r"(v));
}

static __always_inline void
cli(void)
{
	__asm__ __volatile__("cli");
}

static __always_inline void
sti(void)
{
	__asm__ __volatile__("sti");
}

static __always_inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
	uint32_t result;

	// The + in "+m" denotes a read-modify-write operand.
	__asm__ __volatile__("lock; xchgl %0, %1"
											 : "+m"(*addr), "=a"(result)
											 : "1"(newval)
											 : "cc");
	return result;
}

static __always_inline uint32_t
rcr2(void)
{
	uint32_t val;
	__asm__ __volatile__("movl %%cr2,%0" : "=r"(val));
	return val;
}

static __always_inline void
lcr3(uint32_t val)
{
	__asm__ __volatile__("movl %0,%%cr3" : : "r"(val));
}

static __always_inline void
hlt(void)
{
	__asm__ __volatile__("hlt");
}

static __always_inline void
cpuid(uint32_t id, uint32_t count, uint32_t *a, uint32_t *b, uint32_t *c,
			uint32_t *d)
{
	__asm__ __volatile__("movl %0, %%eax\t\n"
											 "cpuid\t\n"
											 : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
											 : "0"(id), "2"(count));
}

// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
	// registers as pushed by pusha
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t oesp; // useless & ignored
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;

	// rest of trap frame
	uint16_t gs;
	uint16_t padding1;
	uint16_t fs;
	uint16_t padding2;
	uint16_t es;
	uint16_t padding3;
	uint16_t ds;
	uint16_t padding4;
	uint32_t trapno;

	// below here defined by x86 hardware
	uint32_t err;
	uint32_t eip;
	uint16_t cs;
	uint16_t padding5;
	uint32_t eflags;

	// below here only when crossing rings, such as from user to kernel
	uint32_t esp;
	uint16_t ss;
	uint16_t padding6;
};
