#pragma once
/* Exported to userspace */
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_SHARED 0x1

#if __KERNEL__
#include "file.h"
#include <stddef.h>
#include <stdint.h>
// Stuff returned by the VFS as "recommendations".
struct mmap_info {
	size_t length;
	// PHYSICAL address of the thing.
	uintptr_t addr;
	// VIRTUAL address of the thing.
	uintptr_t virt_addr;
	struct file *file;
	int perm;
};

#endif
