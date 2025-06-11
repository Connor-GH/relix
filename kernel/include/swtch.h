#pragma once
#if __KERNEL__
#include <proc.h>
void swtch(struct context **, struct context *);
#endif
