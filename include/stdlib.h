#pragma once
#include <types.h>
#define NULL ((void *)0)

__attribute__((malloc))
void *
malloc(uint);
void
free(void *);
int
atoi(const char *);
int
atoi_base(const char *, uint base);
