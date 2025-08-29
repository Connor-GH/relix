#pragma once
#ifdef USE_HOST_TOOLS
#include "include/bits/size_t.h"
#include "include/bits/types.h"
#else
#include <bits/size_t.h>
#include <bits/types.h>
#endif
typedef __ssize_t ssize_t;
typedef __ssize_t ssize_t;

typedef __time_t time_t;
typedef __suseconds_t suseconds_t;
typedef __clock_t clock_t;
typedef __clockid_t clockid_t;
typedef __nlink_t nlink_t;
typedef __blkcnt_t blkcnt_t;

typedef __size_t size_t;

typedef __pid_t pid_t;
typedef __uid_t uid_t;
typedef __gid_t gid_t;
typedef __id_t id_t;

typedef __ino_t ino_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;

typedef __dev_t dev_t;

typedef __off_t off_t;
typedef __mode_t mode_t;

typedef __bits16_t bits16_t;
typedef __bits32_t bits32_t;
typedef __u_bits32_t u_bits32_t;
typedef __bits64_t bits64_t;
typedef __u_int u_int;
typedef __u_long u_long;
