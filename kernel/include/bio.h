#pragma once
#include <stdint.h>

void
block_init(void);
struct buf *
block_read(uint32_t, uint32_t);
void
block_release(struct buf *);
void
block_write(struct buf *);
