#pragma once
#include <stddef.h>
#include <stdint.h>
#include "file.h"
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_SHARED 0x1

// Stuff returned by block devices as "recommendations" if it is a block device.
struct mmap_info {
	size_t length;
	// PHYSICAL address of the thing.
	uintptr_t addr;
	// VIRTUAL address of the thing.
	uintptr_t virt_addr;
	struct file *file;
	int perm;
};

