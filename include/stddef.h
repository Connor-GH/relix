#pragma once
#include <bits/NULL.h>
#include <bits/size_t.h>
#define offsetof(x, y) __builtin_offsetof(x, y)
typedef __size_t size_t;
typedef __typeof__((void *)1 - (void *)0) ptrdiff_t;
#define unreachable() (__builtin_unreachable())
