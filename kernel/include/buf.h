#pragma once
#include <stdint.h>
#include "sleeplock.h"
#include "fs.h"
struct buf {
	int flags;
	dev_t dev;
	ssize_t blockno;
	struct sleeplock lock;
	uint32_t refcnt;
	struct buf *prev; // LRU cache list
	struct buf *next;
	struct buf *qnext; // disk queue
	uint8_t data[BSIZE];
};
#define B_VALID 0x2 // buffer has been read from disk
#define B_DIRTY 0x4 // buffer needs to be written to disk
