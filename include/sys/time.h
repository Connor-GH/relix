#pragma once
#include <bits/types.h>
typedef unsigned long long clock_t;
typedef __suseconds_t suseconds_t;
typedef __time_t time_t;

struct timeval {
	__time_t tv_sec;
	suseconds_t tv_usec;
};
