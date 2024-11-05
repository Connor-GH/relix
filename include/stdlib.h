#pragma once

#ifndef __KERNEL__
#include <stdint.h>
__attribute__((malloc)) void *malloc(uint32_t);
void
free(void *);
int
atoi(const char *);
int
atoi_base(const char *, uint32_t base);
__attribute__((malloc)) void *
realloc(void *ptr, size_t size);
#endif
