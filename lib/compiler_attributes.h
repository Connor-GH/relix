#pragma once
#if defined(__GNUC__) && !defined(__clang__)
#include "gcc.h"
#elif defined(__clang__)
#include "clang.h"
#else
#error "Unknown compiler!"
#endif

#ifdef __CHECKER__
#define __must_hold(x) __attribute__((context(x, 1, 1)))
#define __acquires(x) __attribute__((context(x, 0, 1)))
#define __releases(x) __attribute__((context(x, 1, 0)))
#define __acquire(x) __context__(x, 1)
#define __release(x) __context__(x, -1)
#else
#define __must_hold(x)
#define __acquires(x)
#define __releases(x)
#define __acquire(x)
#define __release(x)
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
#define __unused __attribute__((unused))
#define __deprecated(msg) __attribute__((deprecated(msg)))
#define __suppress_sanitizer(x) __attribute__((no_sanitize(x)))

#if __has_builtin(__builtin_constant_p) && __has_builtin(__builtin_expect)
#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif
