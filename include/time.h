#pragma once
#include <date.h>
#include <stdint.h>
#include "kernel/include/time.h"
typedef uint64_t time_t;

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
extern uint64_t rtc_to_epoch(struct rtcdate rtc);
#define RTC_TO_UNIX(rtc) rtc_to_epoch(rtc)
// clang-format on
