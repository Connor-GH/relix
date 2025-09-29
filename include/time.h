#pragma once
#include <bits/__NULL.h>
#include <bits/size_t.h>
#include <bits/stdint.h>
#include <bits/struct_timespec.h>
#include <bits/types.h>

#define NULL __NULL

typedef __useconds_t useconds_t;

typedef __pid_t pid_t;
typedef __time_t time_t;
typedef __clockid_t clockid_t;
typedef __clock_t clock_t;
typedef __size_t size_t;

#define CLOCK_MONOTONIC 1

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
time_t time(time_t *tloc);
struct tm *localtime(const time_t *timep);
void tzset(void);
int clock_gettime(clockid_t clock_id, struct timespec *tp);
int nanosleep(const struct timespec *duration, struct timespec *rem);

extern int daylight;
extern long timezone;
extern char *tzname[2];
