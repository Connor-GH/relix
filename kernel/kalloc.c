// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "kalloc.h"
#include "boot/multiboot2.h"
#include "console.h"
#include "drivers/memlayout.h"
#include "drivers/mmu.h"
#include "kernel_ld_syms.h"
#include "lib/compiler_attributes.h"
#include "macros.h"
#include "spinlock.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void freerange(void *vstart, void *vend);

struct run {
	struct run *next;
};

static struct {
	struct spinlock lock;
	struct run *freelist;
	bool use_lock;
} kmem;

// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
	initlock(&kmem.lock, "kmem");
	kmem.use_lock = false;
	freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
	freerange(vstart, vend);
	kmem.use_lock = true;
}

void
freerange(void *vstart, void *vend)
{
	char *p = (char *)PGROUNDUP((uintptr_t)vstart);
	// If this fails, this will cause a page fault
	// instead of a panic due to the memory for the VGA not being mapped.
	for (; p + PGSIZE <= (char *)vend; p += PGSIZE) {
		kpage_free(p);
	}
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kpage_alloc().	(The exception is when
// initializing the allocator; see kinit above.)
//
// It is also possible that kpage_free MIGHT be NULL.
void
kpage_free(char *v) __releases(kpage)
{
	struct run *r;
	// We do not allocate for devices.
	if (V2IO(v) >= DEVBASE) {
		return;
	}

	if ((uintptr_t)v % PGSIZE || v < __kernel_end ||
	    (V2P(v) >= available_memory && likely(available_memory > 0))) {
		panic("kpage_free");
	}

	if (kmem.use_lock) {
		acquire(&kmem.lock);
	}

	r = (struct run *)v;
	r->next = kmem.freelist;
	kmem.freelist = r;

	if (kmem.use_lock) {
		release(&kmem.lock);
	}
	__release(kpage);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *
kpage_alloc(void) __acquires(kpage)
{
	struct run *r;

	if (kmem.use_lock) {
		acquire(&kmem.lock);
	}
	r = kmem.freelist;

	if (r != NULL) {
		kmem.freelist = r->next;
	}
	if (kmem.use_lock) {
		release(&kmem.lock);
	}

	__acquire(kpage);
	return (char *)r;
}

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

struct header {
	struct header *ptr; // Next free list.
	size_t size;
};

typedef struct header Header;

#define MORECORE_MAX 4096
static Header base; // Empty list to get started.
static Header *freep; // Start of free list.

#define ptr_to_header(ptr) (((Header *)(ptr)) - 1)
#define header_to_ptr(hdr) ((void *)((hdr) + 1))

void
kfree(void *ap) __releases(kmem)
{
	Header *bp, *p;

	if (ap == NULL) {
		uart_printf("WARN: kernel tried to free a NULL pointer\n");
		return;
	}

	bp = ptr_to_header(ap);
	for (p = freep; p && !(bp > p && bp < p->ptr); p = p->ptr) {
		if (p >= p->ptr && (bp > p || bp < p->ptr)) {
			break;
		}
	}

	if (!p) {
		return;
	}

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
	}
	freep = p;
	__release(kmem);
}

static Header *
morecore(size_t nu)
{
	char *p;
	Header *hp;

	if (nu < MORECORE_MAX) {
		nu = MORECORE_MAX;
	}

	p = kpage_alloc();
	if (p == NULL) {
		return NULL;
	}
	hp = (Header *)p;
	hp->size = 4096 / sizeof(Header); // kalloc always allocates 4096 bytes
	kfree(header_to_ptr(hp));
	return freep;
}

// TODO we should make an aligned malloc.
void *
kmalloc(size_t nbytes) __acquires(kmem)
{
	Header *prevp;
	size_t nunits;

	if (nbytes == 0) {
		uart_printf("WARN: kernel tried to allocate memory with size 0.\n");
		return NULL;
	}

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

	// No free list.
	if ((prevp = freep) == NULL) {
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}
	for (Header *p = prevp->ptr;; prevp = p, p = p->ptr) {
		if (p == NULL) {
			return NULL;
		}
		// We found a size that can fit the amount of bytes we want.
		if (p->size >= nunits) {
			// It is exactly the right size.
			if (p->size == nunits) {
				prevp->ptr = p->ptr;
			} else {
				// Too large? Okay, split it into chunks
				// and give ourselves the larger piece.
				p->size -= nunits;
				// Go to the header we just said is ours.
				p += p->size;
				// Assign our size.
				p->size = nunits;
			}
			freep = prevp;
			__acquire(kmem);
			return header_to_ptr(p);
		}
		if (p == freep) {
			if ((p = morecore(nunits)) == NULL) {
				return NULL;
			}
		}
	}
}

__attribute__((malloc)) void *
krealloc(void *ptr, size_t size)
{
	if (ptr == NULL) {
		return kmalloc(size);
	}

	Header *hdr = ptr_to_header(ptr);
	void *newptr = kmalloc(size);
	if (newptr == NULL) {
		return NULL;
	}

	memcpy(newptr, ptr, min(hdr->size, size));
	kfree(ptr);

	return newptr;
}

__attribute__((malloc)) void *
kcalloc(size_t size)
{
	void *ptr = kmalloc(size);
	if (ptr == NULL) {
		return NULL;
	}
	memset(ptr, '\0', size);
	return ptr;
}
