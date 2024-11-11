#pragma once
#include <stdint.h>
// Memory layout
#define kiB (1024ULL)
#define MiB (1024 * kiB)
#define GiB (1024 * MiB)

#define EXTMEM 0x100000 // Start of extended memory
#define PHYSTOP (256 * MiB - (32 * MiB)) //0xE000000 // Top physical memory
#define DEVSPACE 0xFE000000 // Other devices are at high addresses

// Key addresses for address space layout (see kmap in vm.c for layout)
#define KERNLINK (KERNBASE + EXTMEM) // Address where kernel is linked

#define V2P(a) (((uintptr_t)(a)) - KERNBASE)
#define P2V(a) ((void *)(((char *)(a)) + KERNBASE))

#define V2P_WO(x) ((x) - KERNBASE) // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE) // same as P2V, but without casts

// Key addresses for address space layout (see kmap in vm.c for layout)
#ifdef X64
#define KERNBASE 0xFFFFFFFF80000000ULL // First kernel virtual address
#define DEVBASE 0xFFFFFFFF40000000ULL // First device virtual address
#else
#define KERNBASE 0x80000000UL // First kernel virtual address
#define DEVBASE 0xFE000000UL // First device virtual address
#endif

#ifndef __ASSEMBLER__

static inline uintptr_t
v2p(void *a)
{
	return ((uintptr_t)(a)) - ((uintptr_t)KERNBASE);
}
static inline void *
p2v(uintptr_t a)
{
	return (void *)((a) + ((uintptr_t)KERNBASE));
}

#endif

#define IO2V(a) (((void *)(a)) + DEVBASE - DEVSPACE)
