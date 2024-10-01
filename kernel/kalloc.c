// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include <types.h>
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
#include "kernel_string.h"

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
	p = (char *)PGROUNDUP((uint)vstart);
	for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
		kfree(p);
	popcli();
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
__nonnull(1) void kfree(char *v)
{
	pushcli();
	struct run *r;
	int id = my_cpu_id();

	if ((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
		panic("kfree");

	// Fill with junk to catch dangling refs.
	memset(v, 1, PGSIZE);

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
kalloc(void)
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

	if (r)
		memset((char *)r, 5, PGSIZE);
	popcli();
	return (char *)r;
}
__attribute__((malloc)) __nonnull(1) void *
krealloc(char *ptr, size_t size)
{
	if (ptr) kfree(ptr);
	ptr = kalloc();
	return ptr;
}
