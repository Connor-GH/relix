#include "stdint.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.


struct header {
	struct header *ptr;
	size_t size;
} __attribute__((aligned(8)));

typedef struct header Header;

static Header base;
static Header *freep;

void
free(void *ap)
{
	Header *bp, *p;

#if defined(MEMORY_GUARDS)
	if (ap == NULL) {
		fprintf(stderr, "free(NULL) was called.\n");
		abort();
	}
#endif

	bp = (Header *)ap - 1;
	for (p = freep; !(bp > p && bp < p->ptr); p = p->ptr)
		if (p >= p->ptr && (bp > p || bp < p->ptr))
			break;

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
}

#define MORECORE_THRESHOLD 4096
// "nu" is "number of units".
//
static Header *
morecore(size_t nu)
{
	char *p;
	Header *hp;

	if (nu < MORECORE_THRESHOLD) {
		nu = MORECORE_THRESHOLD;
	}
	p = sbrk(nu * sizeof(Header));
	if (p == (char *)-1)
		return NULL;
	hp = (Header *)p;
	hp->size = nu;
	free((void *)(hp + 1));
	return freep;
}

__attribute__((malloc)) void *
malloc(size_t nbytes)
{
	Header *p, *prevp;
	size_t nunits;
#if defined(MEMORY_GUARDS)
	if (nbytes == 0) {
		fprintf(stderr, "malloc(0) is undefined behavior.\n");
		abort();
	}
#endif
	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
	// There is no free list yet, so we need to make one.
	if ((prevp = freep) == NULL) {
		base.ptr = freep = prevp = &base;
		base.size = 0;
	}
	for (p = prevp->ptr;; prevp = p, p = p->ptr) {
		// This pointer size (p->size) is big enough.
		if (p->size >= nunits) {
			// The rare case where it is exactly big enough.
			if (p->size == nunits) {
				prevp->ptr = p->ptr;
			} else {
				// Use the end part of p for nunits storage.
				// memory: [| ... ... ... ]
				//          ^~~ p pointer
				// 				 [ ... ... | ]
				//    p += p->size ~~^
				p->size -= nunits;
				p += p->size;
				p->size = nunits;
			}
			freep = prevp;
			return (void *)(p + 1);
		}
		if (p == freep)
			if ((p = morecore(nunits)) == NULL)
				return NULL;
	}
}
__attribute__((malloc)) void *
realloc(void *ptr, size_t size)
{
	if (ptr)
		free(ptr);
	ptr = malloc(size);
	return ptr;
}
__attribute__((malloc)) void *
calloc(size_t nmemb, size_t sz)
{
	void *ptr = malloc(nmemb * sz);
	if (ptr == NULL)
		return NULL;
	memset(ptr, 0, nmemb * sz);
	return ptr;
}
