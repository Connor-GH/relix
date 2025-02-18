#pragma once
#define NULL ((void *)0)
#define offsetof(x, y) __builtin_offsetof(x, y)
typedef __typeof__(sizeof(0)) size_t;
typedef int wchar_t;
typedef typeof((void *)1 - (void *)0) ptrdiff_t;
