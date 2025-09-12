#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

struct header {
	struct header *ptr;
	size_t size;
} __attribute__((aligned(8)));

typedef struct header Header;

static Header base = { NULL, 0 };
static Header *freep = NULL;

#define ptr_to_header(ptr) (((Header *)ptr) - 1)
#define header_to_ptr(hdr) ((void *)(hdr + 1))

// Undefined behavior:
// - the pointer we are handed is not from {re,m}alloc.
// - the pointer is from {re,m}alloc, but has been free()'d
void
free(void *ap)
{
	Header *bp, *p;

	// C spec: "If ptr is a null pointer, no action occurs".
	if (ap == NULL) {
		return;
	}

	bp = (Header *)ap - 1;
	for (p = freep; !(bp > p && bp < p->ptr); p = p->ptr) {
		if (p >= p->ptr && (bp > p || bp < p->ptr)) {
			break;
		}
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
}

#define MORECORE_THRESHOLD 4096
// "nu" is "number of units".
//
static Header *
allocate_units(size_t nu)
{
	char *p;
	Header *hp;

	if (nu < MORECORE_THRESHOLD) {
		nu = MORECORE_THRESHOLD;
	}
	// TODO this will be uncommented when the full
	// sbrk->mmap migration is complete.
	// p = mmap(NULL, nu * sizeof(Header), PROT_READ | PROT_WRITE, MAP_ANONYMOUS,
	// -1, 0);
	p = sbrk(nu * sizeof(Header));
	if (p == (char *)-1) {
		return NULL;
	}
	hp = (Header *)p;
	hp->size = nu;
	free((void *)(hp + 1));
	return freep;
}

// Undefined behavior:
// - the the size is 0
__attribute__((malloc)) void *
malloc(size_t nbytes)
{
	Header *p, *prevp;
	size_t nunits;
	if (nbytes == 0) {
		return NULL;
	}
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
		if (p == freep) {
			if ((p = allocate_units(nunits)) == NULL) {
				return NULL;
			}
		}
	}
}

// Undefined behavior:
// - the pointer we are handed is not from {re,m}alloc.
// - the pointer is from {re,m}alloc, but has been free()'d
// - size is 0
__attribute__((malloc)) void *
realloc(void *ptr, size_t size)
{
	// C spec: "If ptr is a null pointer, the realloc function behaves
	// like the malloc function for the specified size"
	if (ptr == NULL) {
		return malloc(size);
	}

	Header *old_ptr_hdr = ptr_to_header(ptr);
	void *new_ptr = malloc(size);

	// C spec:
	// - "If memory for the new object is not allocated, the
	// old object is not deallocated and its value is unchanged."
	//
	// - "The realloc function returns ... a null pointer if the new
	// object has not been allocated".
	if (new_ptr == NULL) {
		return NULL;
	}
	memcpy(new_ptr, ptr, MIN(old_ptr_hdr->size, size));
	free(ptr);
	return new_ptr;
}

// Undefined behavior:
// - either nmemb or sz are 0.
__attribute__((malloc)) void *
calloc(size_t nmemb, size_t sz)
{
	size_t res;
	// C spec: "The calloc function returns ... a null pointer if the
	// space cannot be allocated or if the product nmemb * size would
	// wraparound size_t".
	if (ckd_mul(&res, nmemb, sz)) {
		return NULL;
	}

	void *ptr = malloc(res);
	if (ptr == NULL) {
		return NULL;
	}
	memset(ptr, 0, res);
	return ptr;
}
