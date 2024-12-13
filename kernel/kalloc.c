// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include <stdlib.h>
#include <stdint.h>
#include <stdint.h>
#include "compiler_attributes.h"
#include "drivers/memlayout.h"
#include "drivers/mmu.h"
#include "spinlock.h"
#include "kalloc.h"
#include "console.h"
#include "param.h"
#include "proc.h"
#include "macros.h"
#include <stddef.h>
#include "boot/multiboot2.h"
#include <string.h>

void
freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
	// defined by the kernel linker script in kernel.ld

struct run {
	struct run *next;
};

struct {
	struct spinlock lock[NCPU];
	struct run *freelist[NCPU];
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
	freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
	for (int i = 0; i < min(NCPU, ncpu); i++)
		initlock(&kmem.lock[i], "kmem");
	freerange(vstart, vend);
}

void
freerange(void *vstart, void *vend)
{
	pushcli();
	char *p;
	p = (char *)PGROUNDUP((uintptr_t)vstart);
	for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
		kpage_free(p);
	popcli();
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kpage_alloc().	(The exception is when
// initializing the allocator; see kinit above.)
__nonnull(1) void kpage_free(char *v)
{
	pushcli();
	struct run *r;
	int id = my_cpu_id();

	if ((uintptr_t)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
		panic("kpage_free");


	//if (kmem.use_lock)
	acquire(&kmem.lock[id]);
	r = (struct run *)v;
	r->next = kmem.freelist[id];
	kmem.freelist[id] = r;
	//if (kmem.use_lock)
	release(&kmem.lock[id]);
	popcli();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *
kpage_alloc(void)
{
	pushcli();
	int id = my_cpu_id();
	struct run *r;

	//if (kmem.use_lock)
	acquire(&kmem.lock[id]);
	r = kmem.freelist[id];
	if (r)
		kmem.freelist[id] = r->next;
	//if (kmem.use_lock)
	release(&kmem.lock[id]);

	if (!r) {
		for (int i = 0; i < min(NCPU, ncpu); i++) {
			acquire(&kmem.lock[i]);
			r = kmem.freelist[i];
			if (r)
				kmem.freelist[i] = r->next;
			release(&kmem.lock[i]);

			if (r)
				break;
		}
	}

	popcli();
	return (char *)r;
}
__attribute__((malloc)) __nonnull(1) void *krealloc(void *ptr, size_t size)
{
	if (ptr)
		kfree(ptr);
	ptr = kmalloc(size);
	return ptr;
}

__attribute__((malloc)) void *kcalloc(size_t size) {
	void *ptr = kmalloc(size);
	memset(ptr, '\0', size);
	return ptr;
}

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header {
	struct {
		union header *ptr;
		uint32_t size;
	} s;
	Align x;
};

typedef union header Header;

static Header base;
static Header *freep;

void
kfree(void *ap)
{
	Header *bp, *p;

	bp = (Header *)ap - 1;
	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break;

	if (bp + bp->s.size == p->s.ptr) {
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp) {
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;
	freep = p;
}

static Header *
morecore(uint32_t nu)
{
	char *p;
	Header *hp;

	p = kpage_alloc();
	if (p == 0)
		return 0;
	hp = (Header *)p;
	hp->s.size = 4096 / sizeof(Header); // kalloc always allocates 4096 bytes
	kfree((void *)(hp + 1));
	return freep;
}

void *
kmalloc(size_t nbytes)
{
	Header *p, *prevp;
	uint32_t nunits;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	if ((prevp = freep) == 0) {
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {
			if (p->s.size == nunits)
				prevp->s.ptr = p->s.ptr;
			else {
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *)(p + 1);
		}
		if (p == freep)
			if ((p = morecore(nunits)) == 0)
				return 0;
	}
}
