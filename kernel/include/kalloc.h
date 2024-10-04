#pragma once
#include <stdint.h>
#include "compiler_attributes.h"
char *
kpage_alloc(void);
__nonnull(1) void kpage_free(char *);
__attribute__((malloc)) __nonnull(1) void *
kpage_realloc(char *ptr, size_t size);

__attribute__((malloc)) void *
kmalloc(size_t size);

__nonnull(1)
void
kfree(void *ptr);

void
kinit1(void *, void *);
void
kinit2(void *, void *);
