#pragma once
/* Exported to userspace */

#include <stdint.h>
#ifndef USE_HOST_TOOLS
#include <wordsize.h>
#if __WORDSIZE == 32
typedef int32_t ssize_t;
#else
typedef int64_t ssize_t;
#endif
#endif
typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef uint32_t ino_t;
typedef int id_t;
typedef uint64_t dev_t;

typedef ssize_t off_t;

typedef int mode_t;
typedef short bits16_t;
typedef int bits32_t;
typedef unsigned int u_bits32_t;
typedef long bits64_t;
typedef unsigned int u_int;
typedef unsigned long u_long;
