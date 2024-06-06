#pragma once
#include "types.h"
#include "date.h"
typedef uint time_t;

time_t
time(time_t *tloc);


// clang-format off
#define RTC_TO_UNIX(rtc)                                               \
	(time_t)(rtc.second \
+ (rtc.minute * 60) \
+ (rtc.hour * (60 * 60)) \
+ (rtc.day * (60 * 60 * 24)) \
+ (rtc.month * (60 * 60 * 24 * 30)) \
+ ((rtc.year-2000) * (60 * 60 * 24 * 365)))
// clang-format on
