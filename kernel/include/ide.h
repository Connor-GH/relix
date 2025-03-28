#pragma once
#if __KERNEL__
#include <buf.h>

void
ideinit(void);
void
ideintr(void);
void
iderw(struct buf *);
#endif
