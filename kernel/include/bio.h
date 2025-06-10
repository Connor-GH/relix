#pragma once
#if __KERNEL__
#include <stdint.h>
#include "types.h"
#include "lib/compiler_attributes.h"

void
block_init(void);
struct block_buffer *
block_read(dev_t dev, uint64_t blockno) __acquires(&b->lock);
void
block_release(struct block_buffer *b) __releases(&b->lock);
void
block_write(struct block_buffer *b) __must_hold(&b->lock);
#endif
