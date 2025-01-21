#pragma once
#include <stdint.h>
struct rtcdate {
	uint64_t second;
	uint64_t minute;
	uint64_t hour;
	uint64_t day;
	uint64_t month;
	uint64_t year;
};
