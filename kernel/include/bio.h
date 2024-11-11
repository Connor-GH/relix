#pragma once
#include <stdint.h>

void
binit(void);
struct buf *bread(uint32_t, uint32_t);
void
brelse(struct buf *);
void
bwrite(struct buf *);
