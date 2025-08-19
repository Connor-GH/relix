#pragma once
#include "lib/compiler_attributes.h"
#include <stddef.h>
#include <stdint.h>
// Routines to let C code use special x86 instructions.

static __always_inline double
__fabs(double x)
{
	double result;
	__asm__ __volatile__("fldl %1\t\n" // "Float load" -- add value onto FPU stack
	                     "fabs\t\n"
	                     "fstpl %0\t\n"
	                     : "=m"(result)
	                     : "m"(x)
	                     : "st" // FPU stack clobbered
	);
	return result;
}

static __always_inline uint8_t
inb(uint16_t port)
{
	uint8_t data;

	__asm__ __volatile__("in %1,%0" : "=a"(data) : "d"(port));
	return data;
}

static __always_inline void
insw(int port, void *addr, int cnt)
{
	__asm__ __volatile__("cld; rep insw"
	                     : "=D"(addr), "=c"(cnt)
	                     : "d"(port), "0"(addr), "1"(cnt)
	                     : "memory", "cc");
}

static __always_inline void
insl(int port, void *addr, int cnt)
{
	__asm__ __volatile__("cld; rep insl"
	                     : "=D"(addr), "=c"(cnt)
	                     : "d"(port), "0"(addr), "1"(cnt)
	                     : "memory", "cc");
}

static __always_inline uint32_t
inl(uint16_t port)
{
	uint32_t data;

	__asm__ __volatile__("inl %1,%0" : "=a"(data) : "d"(port));
	return data;
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
outl(uint16_t port, uint32_t data)
{
	__asm__ __volatile__("outl %0,%1" : : "a"(data), "d"(port));
}

static __always_inline uint64_t
xgetbv(uint32_t ecx)
{
	uint32_t eax, edx;
	__asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(ecx));
	return (uint64_t)edx << 32 | (uint64_t)eax;
}

static __always_inline void
xsetbv(uint32_t ecx, uint64_t value)
{
	uint32_t edx = value >> 32;
	uint32_t eax = (uint32_t)value;
	__asm__ __volatile__("xsetbv" : : "a"(eax), "c"(ecx), "d"(edx));
}

static __always_inline uint64_t
read_cr4(void)
{
	uint64_t cr4;
	__asm__ __volatile__("mov %%cr4, %0" : "=r"(cr4));
	return cr4;
}

static __always_inline void
write_cr4(uint64_t value)
{
	__asm__ __volatile__("mov %0, %%cr4" : : "r"(value));
}

static __always_inline uint64_t
read_cr0(void)
{
	uint64_t cr0;
	__asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
	return cr0;
}

static __always_inline void
write_cr0(uint64_t value)
{
	__asm__ __volatile__("mov %0, %%cr0" : : "r"(value));
}

static __always_inline void
stosb(void *addr, uint8_t data, size_t cnt)
{
	__asm__ __volatile__("cld; rep stosb"
	                     : "=D"(addr), "=c"(cnt)
	                     : "0"(addr), "1"(cnt), "a"(data)
	                     : "memory", "cc");
}

static __always_inline void
stosl(void *addr, int data, size_t cnt)
{
	__asm__ __volatile__("cld; rep stosl"
	                     : "=D"(addr), "=c"(cnt)
	                     : "0"(addr), "1"(cnt), "a"(data)
	                     : "memory", "cc");
}
static __always_inline void *
movsq(uint64_t *dst, const uint64_t *src, size_t size)
{
	__asm__ __volatile__("rep movsq"
	                     : "+D"(dst), "+S"(src), "+c"(size)
	                     :
	                     : "memory");
	return dst;
}
static __always_inline void *
movsb(uint8_t *dst, const uint8_t *src, size_t size)
{
	__asm__ __volatile__("rep movsb"
	                     : "+D"(dst), "+S"(src), "+c"(size)
	                     :
	                     : "memory");
	return dst;
}
static __always_inline uint64_t
rdtsc(void)
{
	uint32_t msw, lsw;
	__asm__ __volatile__("rdtsc" : "=d"(msw), "=a"(lsw));
	return ((uint64_t)msw << 32) | lsw;
}

struct segdesc;

static __always_inline void
lgdt(struct segdesc *p, int size)
{
	volatile uint16_t pd[5];

	pd[0] = size - 1;
	pd[1] = (uintptr_t)p;
	pd[2] = (uintptr_t)p >> 16;
#if X86_64
	pd[3] = (uintptr_t)p >> 32;
	pd[4] = (uintptr_t)p >> 48;
#endif

	__asm__ __volatile__("lgdt (%0)" : : "r"(pd));
}

struct gatedesc;

static __always_inline void
lidt(struct gatedesc *p, int size)
{
	volatile uint16_t pd[5];

	pd[0] = size - 1;
	pd[1] = (uintptr_t)p;
	pd[2] = (uintptr_t)p >> 16;
#if X86_64
	pd[3] = (uintptr_t)p >> 32;
	pd[4] = (uintptr_t)p >> 48;
#endif

	__asm__ __volatile__("lidt (%0)" : : "r"(pd));
}

static __always_inline void
ltr(uint16_t sel)
{
	__asm__ __volatile__("ltr %0" : : "r"(sel));
}

static __always_inline uintptr_t
readrflags(void)
{
	uintptr_t rflags;
	__asm__ __volatile__("pushf; pop %0" : "=r"(rflags));
	return rflags;
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

static __always_inline uintptr_t
xchg(volatile uint32_t *addr, uintptr_t newval)
{
	uintptr_t result;

	// The + in "+m" denotes a read-modify-write operand.
	__asm__ __volatile__("lock; xchg %0, %1"
	                     : "+m"(*addr), "=a"(result)
	                     : "1"(newval)
	                     : "cc");
	return result;
}

static __always_inline uintptr_t
rcr2(void)
{
	uintptr_t val;
	__asm__ __volatile__("mov %%cr2,%0" : "=r"(val));
	return val;
}

static __always_inline void
lcr3(uintptr_t val)
{
	__asm__ __volatile__("movq %0,%%cr3" : : "r"(val));
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

static __always_inline uint64_t
rdmsr(uint32_t msr)
{
	uint32_t hi;
	uint32_t lo;
	__asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return (uint64_t)hi << 32 | lo;
}

static __always_inline void
wrmsr(uint32_t msr, uint64_t val)
{
	uint32_t lo, hi;
	lo = val & 0xFFFFFFFF;
	hi = val >> 32 & 0xFFFFFFFF;
	__asm__ __volatile__("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
