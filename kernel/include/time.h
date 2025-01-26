#pragma once
#ifndef USE_HOST_TOOLS
#include "sys/types.h"
#include <stddef.h>
typedef size_t time_t;
typedef size_t useconds_t;
typedef ssize_t suseconds_t;
struct timeval {
	time_t tv_sec;
	suseconds_t tv_usec;
};
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#endif
