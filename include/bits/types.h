#pragma once

#ifndef USE_HOST_TOOLS
#include <bits/stdint.h>
#include <wordsize.h>
#else
#include "include/bits/stdint.h"
#include "include/wordsize.h"
#endif
#if __WORDSIZE == 32
typedef int32_t __ssize_t;
#else
typedef __int64_t __ssize_t;
#endif

typedef __uint64_t __time_t;
typedef __uint64_t __clock_t;
#ifndef USE_HOST_TOOLS
typedef __uint64_t __useconds_t;
#endif
typedef __int32_t __suseconds_t;
typedef int __clockid_t;
typedef int __nlink_t;
typedef __ssize_t __blkcnt_t;

typedef int __pid_t;
typedef int __uid_t;
typedef int __gid_t;
typedef int __id_t;

typedef __uint32_t __ino_t;
typedef __uint64_t __fsblkcnt_t;
typedef __uint64_t __fsfilcnt_t;

typedef __uint64_t __dev_t;

typedef __ssize_t __off_t;
typedef __ssize_t __blksize_t;
typedef unsigned int __mode_t;

typedef short __bits16_t;
typedef int __bits32_t;
typedef unsigned int __u_bits32_t;
typedef long __bits64_t;
typedef unsigned int __u_int;
typedef unsigned long __u_long;
