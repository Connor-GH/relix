#pragma once
#include <stddef.h>
int
strcasecmp(const char *p, const char *q);

int
strncasecmp(const char *p, const char *q, size_t n);

void
bcopy(const void *src, void *dst, size_t n);
