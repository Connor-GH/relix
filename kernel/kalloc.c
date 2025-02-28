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
	// If this fails, this will cause a page fault
	// instead of a panic due to the memory for the VGA not being mapped.
	for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
		kpage_free(p);
	popcli();
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kpage_alloc().	(The exception is when
// initializing the allocator; see kinit above.)
__nonnull(1) void kpage_free(char *v) __releases(kpage)
{
	pushcli();
	struct run *r;
	int id = my_cpu_id();
	// We do not allocate for devices.
	if (V2IO(v) >= DEVBASE)
		return;

	if ((uintptr_t)v % PGSIZE || v < end ||
		(V2P(v) >= available_memory && likely(available_memory > 0)))
		panic("kpage_free");

	//if (kmem.use_lock)
	acquire(&kmem.lock[id]);
	r = (struct run *)v;
	r->next = kmem.freelist[id];
	kmem.freelist[id] = r;
	//if (kmem.use_lock)
	release(&kmem.lock[id]);
	popcli();
	__release(kpage);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *
kpage_alloc(void) __acquires(kpage)
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
	__acquire(kpage);
	return (char *)r;
}

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

struct header {
		struct header *ptr;
		size_t size;
} __attribute__((aligned(8)));

typedef struct header Header;

static Header base;
static Header *freep;

void
kfree(void *ap) __releases(kmem)
{
	Header *bp, *p;

	bp = (Header *)ap - 1;
	for (p = freep; p && !(bp > p && bp < p->ptr); p = p->ptr)
		if (p >= p->ptr && (bp > p || bp < p->ptr))
			break;

	if (!p)
		return;
	if (bp + bp->size == p->ptr) {
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	} else {
		bp->ptr = p->ptr;
	}
	if (p + p->size == bp) {
		p->size += bp->size;
		p->ptr = bp->ptr;
	} else {
		p->ptr = bp;
	freep = p;
	}
	__release(kmem);
}

static Header *
morecore(__attribute__((unused)) size_t nu)
{
	char *p;
	Header *hp;

	p = kpage_alloc();
	if (p == 0)
		return 0;
	hp = (Header *)p;
	hp->size = 4096 / sizeof(Header); // kalloc always allocates 4096 bytes
	kfree((void *)(hp + 1));
	return freep;
}

void *
kmalloc(size_t nbytes) __acquires(kmem)
{
	Header *p, *prevp;
	size_t nunits;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	if ((prevp = freep) == 0) {
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}
	for (p = prevp->ptr;; prevp = p, p = p->ptr) {
		if (!p)
			return 0;
		if (p->size >= nunits) {
			if (p->size == nunits)
				prevp->ptr = p->ptr;
			else {
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}
			freep = prevp;
			__acquire(kmem);
			return (void *)(p + 1);
		}
		if (p == freep)
			if ((p = morecore(nunits)) == 0)
				return 0;
	}
}
__attribute__((malloc)) __nonnull(1) void *krealloc(void *ptr, size_t size)
{
	void *newptr = kmalloc(size);
	if (!newptr)
		return NULL;
	Header *hdr = (Header *)ptr - 1;
	memcpy(newptr, ptr, min(hdr->size, size));
	kfree(ptr);

	return newptr;
}

__attribute__((malloc)) void *
kcalloc(size_t size)
{
	void *ptr = kmalloc(size);
	if (ptr == NULL)
		return NULL;
	memset(ptr, '\0', size);
	return ptr;
}
