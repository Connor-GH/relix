#pragma once

// for compat once we split out these headers more
#include <date.h>

int
uptime(void);
void
echoout(int answer);

// time? more like a nonstandard time(time_t *)
int
date(struct rtcdate *);
