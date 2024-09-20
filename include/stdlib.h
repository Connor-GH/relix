#pragma once
#include <types.h>
#include <stdint.h>
#define NULL ((void *)0)

#ifndef __KERNEL__
__attribute__((malloc)) void *malloc(uint);
void
free(void *);
int
atoi(const char *);
int
atoi_base(const char *, uint base);
__attribute__((malloc)) void *
realloc(void *ptr, size_t size);
#endif
