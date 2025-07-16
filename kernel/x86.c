#include <stddef.h>
#include <stdint.h>
#include <x86.h>

// Routines to let C code use special x86 instructions.
// This is only used by Rust for bindgen.

uint8_t
inb__extern(uint16_t port)
{
	return inb(port);
}

void
insw__extern(int port, void *addr, int cnt)
{
	return insw(port, addr, cnt);
}

void
insl__extern(int port, void *addr, int cnt)
{
	return insl(port, addr, cnt);
}

uint32_t
inl__extern(uint16_t port)
{
	return inl(port);
}

void
outb__extern(uint16_t port, uint8_t data)
{
	return outb(port, data);
}

void
outw__extern(uint16_t port, uint16_t data)
{
	return outw(port, data);
}

void
outsl__extern(int port, const void *addr, int cnt)
{
	return outsl(port, addr, cnt);
}

void
outl__extern(uint16_t port, uint32_t data)
{
	return outl(port, data);
}

uint64_t
xgetbv__extern(uint32_t ecx)
{
	return xgetbv(ecx);
}

void
xsetbv__extern(uint32_t ecx, uint64_t value)
{
	return xsetbv(ecx, value);
}

uint64_t
read_cr4__extern(void)
{
	return read_cr4();
}

void
write_cr4__extern(uint64_t value)
{
	return write_cr4(value);
}

uint64_t
read_cr0__extern(void)
{
	return read_cr0();
}

void
write_cr0__extern(uint64_t value)
{
	return write_cr0(value);
}

void
stosb__extern(void *addr, int data, int cnt)
{
	return stosb(addr, data, cnt);
}

void
stosl__extern(void *addr, int data, int cnt)
{
	return stosl(addr, data, cnt);
}

void *
movsq__extern(uint64_t *dst, uint64_t *src, size_t size)
{
	return movsq(dst, src, size);
}

void *
movsb__extern(uint8_t *dst, uint8_t *src, size_t size)
{
	return movsb(dst, src, size);
}

uint64_t
rdtsc__extern(void)
{
	return rdtsc();
}

struct segdesc;
void
lgdt__extern(struct segdesc *p, int size)
{
	return lgdt(p, size);
}

struct gatedesc;
void
lidt__extern(struct gatedesc *p, int size)
{
	return lidt(p, size);
}

void
ltr__extern(uint16_t sel)
{
	return ltr(sel);
}

uintptr_t
readrflags__extern(void)
{
	return readrflags();
}

void
loadgs__extern(uint16_t v)
{
	return loadgs(v);
}

void
cli__extern(void)
{
	return cli();
}

void
sti__extern(void)
{
	return sti();
}

uintptr_t
xchg__extern(volatile uint32_t *addr, uintptr_t newval)
{
	return xchg(addr, newval);
}

uintptr_t
rcr2__extern(void)
{
	return rcr2();
}

void
lcr3__extern(uintptr_t val)
{
	return lcr3(val);
}

void
hlt__extern(void)
{
	return hlt();
}

void
cpuid__extern(uint32_t id, uint32_t count, uint32_t *a, uint32_t *b,
              uint32_t *c, uint32_t *d)
{
	return cpuid(id, count, a, b, c, d);
}

uint64_t
rdmsr__extern(uint32_t msr)
{
	return rdmsr(msr);
}

void
wrmsr__extern(uint32_t msr, uint64_t val)
{
	return wrmsr(msr, val);
}
