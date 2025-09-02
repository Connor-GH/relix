#pragma once
#include <bits/types.h>
#include <sys/time.h>

typedef __id_t id_t;
typedef unsigned int rlim_t;

struct rlimit {
	rlim_t rlim_cur; // Current (soft) limit
	rlim_t rlim_max; // Max (hard) limit
};

struct rusage {
	struct timeval r_utime; // User time
	struct timeval r_stime; // System time
};
