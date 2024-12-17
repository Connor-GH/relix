#pragma once
#include <stdint.h>
#include <wordsize.h>
#if __WORDSIZE == 32
typedef uint32_t size_t;
typedef int32_t ssize_t;
typedef uint32_t off_t;
#else
typedef uint64_t size_t;
typedef int64_t ssize_t;
typedef uint64_t off_t;
#endif
