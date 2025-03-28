#pragma once
#if __KERNEL__
#include <file.h>

int
pipealloc(struct file **, struct file **);
void
pipeclose(struct pipe *, int);
int
piperead(struct pipe *, char *, int);
int
pipewrite(struct pipe *, char *, int);
#endif
