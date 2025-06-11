#pragma once
#if __KERNEL__
#include "fs.h"
#include "sleeplock.h"
#include <stdint.h>
struct block_buffer {
	int flags;
	dev_t dev;
	ssize_t blockno;
	struct sleeplock lock;
	uint32_t refcnt;
	struct block_buffer *prev; // LRU cache list
	struct block_buffer *next;
	struct block_buffer *qnext; // disk queue
	uint8_t data[BSIZE];
};
#define B_VALID 0x2 // buffer has been read from disk
#define B_DIRTY 0x4 // buffer needs to be written to disk
#endif
