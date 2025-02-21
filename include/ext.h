#pragma once

// for compat once we split out these headers more
#include <date.h>
#include <sys/types.h>

time_t
uptime(void);
int
echoout(int answer);

// time? more like a nonstandard time(time_t *)
int
date(struct rtcdate *);
