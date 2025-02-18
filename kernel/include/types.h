#pragma once
#include <wordsize.h>
#include <stdint.h>
#if __WORDSIZE == 32
typedef int32_t ssize_t;
#else
typedef int64_t ssize_t;
#endif
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef int id_t;

typedef ssize_t off_t;

typedef int mode_t;
typedef short bits16_t;
typedef int bits32_t;
typedef unsigned int u_bits32_t;
typedef long bits64_t;
typedef unsigned int u_int;
typedef unsigned long u_long;
