#pragma once
#include "compiler_attributes.h"
char *
kalloc(void);
__nonnull(1)
void
kfree(char *);
void
kinit1(void *, void *);
void
kinit2(void *, void *);

