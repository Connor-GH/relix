#pragma once
#if __RELIX_KERNEL__
#include <proc.h>
void swtch(struct context **, struct context *);
#endif
