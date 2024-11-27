#pragma once
#define EXIT_SUCCESS 0
#define EXIT_FAILURE !EXIT_SUCCESS
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
int
atoi_base(const char *, uint32_t base);
__attribute__((malloc)) void *
realloc(void *ptr, size_t size);
#endif
