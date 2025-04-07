#pragma once
#if __KERNEL__
#include <file.h>

struct pipe {
	struct spinlock lock;
	int readopen; // read fd is still open
	int writeopen; // write fd is still open
	struct ring_buf *ring_buffer;
};

int
pipealloc(struct file **, struct file **);
void
pipeclose(struct pipe *, int);
int
piperead(struct pipe *, char *, int);
int
pipewrite(struct pipe *, char *, int);
#endif
