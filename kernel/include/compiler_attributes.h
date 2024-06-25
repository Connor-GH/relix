#pragma once

#if defined(__GNUC__) && !defined(__clang__)
#include "gcc.h"
#elif defined(__clang__)
#include "clang.h"
#else
#error "Unknown compiler!"
#endif

#define __always_inline inline __attribute__((__always_inline__))
// this function does not get called often.
#define __cold __attribute__((__cold__))
// every input always has the same output. aka, a->a', b->b', b->NOT a'
#define __const_func __attribute__((__const__))
#define __noreturn __attribute__((__noreturn__))
// run x at the end of this variable's scope
#define __cleanup(x) __attribute__((__cleanup__(x)))
#define __nonnull(...) __attribute__((__nonnull__(__VA_ARGS__)))

#if __has_builtin(__builtin_constant_p) && __has_builtin(__builtin_expect)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
