#pragma once
#include <date.h>
#include <stdint.h>
typedef uint32_t time_t;

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

// clang-format off
// this is a workaround for now.
#define RTC_TO_UNIX(rtc)                                               \
	(time_t)(rtc.second \
+ (rtc.minute * 60) \
+ (rtc.hour * (60 * 60)) \
+ (rtc.day * (60 * 60 * 24)) \
+ (rtc.month * (60 * 60 * 24 * 30.44)) \
+ ((rtc.year-1970) * (60 * 60 * 24 * 365.24))) - 41472
// clang-format on
