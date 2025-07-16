#pragma once
#include <bits/stdint.h>
#include <bits/struct_timespec.h>
#include <bits/types.h>

typedef __uint64_t useconds_t;

typedef __pid_t pid_t;
typedef __time_t time_t;

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

extern int daylight;
extern long timezone;
extern char *tzname[2];
