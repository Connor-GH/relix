#pragma once
/* Exported to userspace */

#include <stddef.h>
#include <stdint.h>
#ifndef USE_HOST_TOOLS
#include <wordsize.h>
#if __WORDSIZE == 32
typedef int32_t ssize_t;
#else
typedef int64_t ssize_t;
#endif

typedef uint64_t time_t;
typedef int32_t suseconds_t;
typedef int nlink_t;
typedef ssize_t blkcnt_t;

#endif

typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
typedef int id_t;

typedef uint32_t ino_t;
typedef uint64_t fsblkcnt_t;
typedef uint64_t fsfilcnt_t;

typedef uint64_t dev_t;

typedef ssize_t off_t;
typedef int mode_t;

typedef short bits16_t;
typedef int bits32_t;
typedef unsigned int u_bits32_t;
typedef long bits64_t;
typedef unsigned int u_int;
typedef unsigned long u_long;
