#pragma once
#include <stdint.h>
struct rtcdate {
	uint32_t second;
	uint32_t minute;
	uint32_t hour;
	uint32_t day;
	uint32_t month;
	uint32_t year;
};
