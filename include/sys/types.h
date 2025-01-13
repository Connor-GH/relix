#pragma once
#include <stdint.h>
#include <wordsize.h>
#if __WORDSIZE == 32
typedef int32_t ssize_t;
typedef uint64_t off_t;
#else
typedef int64_t ssize_t;
#endif
typedef uint64_t off_t;
