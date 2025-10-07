#pragma once
#include <bits/__NULL.h>
#include <bits/size_t.h>
#include <bits/wchar_t.h>
#define offsetof(x, y) __builtin_offsetof(x, y)
#define NULL __NULL
typedef __size_t size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __wchar_t wchar_t;
#define unreachable() (__builtin_unreachable())
