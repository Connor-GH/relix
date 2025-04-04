#pragma once
#include <stddef.h>
#include <sys/types.h>

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
struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
	long tm_gmtoff;
	const char *tm_zone;
};
time_t
time(time_t *tloc);
struct tm *
localtime(const time_t *timep);

extern int daylight;
extern long timezone;
extern char *tzname[2];
