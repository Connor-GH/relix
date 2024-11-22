#pragma once

#ifndef __KERNEL__
#include <stdint.h>
__attribute__((malloc)) void *malloc(uint32_t);
__attribute__((malloc))
void *
calloc(size_t nmemb, uint32_t sz);
void
free(void *);
int
atoi(const char *);
uint64_t
atoi_base(const char *, uint32_t base);
__attribute__((malloc)) void *
realloc(void *ptr, size_t size);
#endif
