#pragma once
// supported by clang 18 and higher
#if __clang_major__ >= 18
#define __counted_by(x) __attribute__((__counted_by__(x)))
#else
#define __counted_by(x)
#endif

#define __retain
