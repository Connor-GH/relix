#pragma once
#include <stdint.h>
#include "compiler_attributes.h"
char *
kalloc(void);
__nonnull(1) void kfree(char *);
__attribute__((malloc)) __nonnull(1) void *
krealloc(char *ptr, size_t size);
void
kinit1(void *, void *);
void
kinit2(void *, void *);
