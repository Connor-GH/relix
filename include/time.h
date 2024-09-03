#pragma once
#include <types.h>
#include <date.h>
typedef uint time_t;

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
