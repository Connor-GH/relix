#pragma once
#include <stddef.h>
#include "compiler_attributes.h"
char *
kpage_alloc(void);
__nonnull(1) void kpage_free(char *);
__attribute__((malloc)) __nonnull(1) void *kpage_realloc(char *ptr,
																												 size_t size);

__attribute__((malloc)) void *
kmalloc(size_t size);
__attribute__((malloc)) void *
kmalloc_aligned(size_t nbytes, size_t alignment);
__nonnull(1) __attribute__((malloc)) void *krealloc(void *ptr, size_t size);
__attribute__((malloc)) void *
kcalloc(size_t size);

__nonnull(1) void kfree(void *ptr);

void
kinit1(void *, void *);
void
kinit2(void *, void *);
