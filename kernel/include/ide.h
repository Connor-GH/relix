#pragma once
#if __RELIX_KERNEL__
#include <buf.h>

void ide_init(void);
void ide_disk_init(void);
void ideintr(void);
void iderw(struct block_buffer *);
#endif
