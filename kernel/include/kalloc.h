#pragma once
#if __RELIX_KERNEL__
#include <stddef.h>
char *kpage_alloc(void);
void kpage_free(char *);
__attribute__((malloc)) void *kpage_realloc(char *ptr, size_t size);

__attribute__((malloc)) void *kmalloc(size_t size);
__attribute__((malloc)) void *kmalloc_aligned(size_t nbytes, size_t alignment);
__attribute__((malloc)) void *krealloc(void *ptr, size_t size);
__attribute__((malloc)) void *kcalloc(size_t size);

void kfree(void *ptr);

void kinit1(void *, void *);
void kinit2(void *, void *);
#endif
