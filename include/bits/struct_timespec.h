#pragma once
#ifndef USE_HOST_TOOLS
#include <bits/types.h>
#else
#include "include/bits/types.h"
#endif
struct timespec {
	__time_t tv_sec;
	long tv_nsec;
};
