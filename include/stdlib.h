#pragma once
#include <types.h>
#define NULL ((void *)0)

void *
malloc(uint);
void
free(void *);
int
atoi(const char *);
int
atoi_base(const char *, uint base);
