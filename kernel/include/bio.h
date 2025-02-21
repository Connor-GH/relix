#pragma once
#include <stdint.h>
#include "types.h"

void
block_init(void);
struct buf *
block_read(dev_t dev, uint64_t blockno);
void
block_release(struct buf *);
void
block_write(struct buf *);
