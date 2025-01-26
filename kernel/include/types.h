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
