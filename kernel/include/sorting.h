#pragma once
#include <stddef.h>
/* From sharedlib.a */
void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *));
