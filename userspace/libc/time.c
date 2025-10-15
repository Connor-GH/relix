/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static struct tm s_time;

int daylight;
long timezone;
char *tzname[2] = { NULL, NULL };

void
tzset(void)
{
	// TODO timezone parsing from TZ environment variable.
	tzname[0] = "   ";
	tzname[1] = "   ";
}

struct tm *
localtime(const time_t *timep)
{
	if (timep == NULL) {
		return NULL;
	}
	time_t unix_seconds = *timep;

	// TODO parse daylight savings time and timezone.
	daylight = 0;
	timezone = 0;
	// Number of days in month in a normal year.
	const int days_of_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	int curr_year, days_till_now, extra_time, extra_days, index, date, month,
		hours, minutes, seconds, flag = 0;

	days_till_now = unix_seconds / (24 * 60 * 60);
	extra_time = unix_seconds % (24 * 60 * 60);
	curr_year = 1970;

	// Calculating current year
	while (true) {
		if (curr_year % 400 == 0 || (curr_year % 4 == 0 && curr_year % 100 != 0)) {
			if (days_till_now < 366) {
				break;
			}
			days_till_now -= 366;
		} else {
			if (days_till_now < 365) {
				break;
			}
			days_till_now -= 365;
		}
		curr_year += 1;
	}

	// Updating extra_days because it
	// will give days till previous day
	// and we have include current day.
	extra_days = days_till_now + 1;

	if (curr_year % 400 == 0 || (curr_year % 4 == 0 && curr_year % 100 != 0)) {
		flag = 1;
	}

	// Calculating month and date.
	month = 0;
	index = 0;
	if (flag == 1) {
		while (index < 12) {
			if (index == 1) {
				if (extra_days - 29 < 0) {
					break;
				}

				month += 1;
				extra_days -= 29;
			} else {
				if (extra_days - days_of_month[index] < 0) {
					break;
				}
				month += 1;
				extra_days -= days_of_month[index];
			}
			index += 1;
		}
	} else {
		while (index < 12) {
			if (extra_days - days_of_month[index] < 0) {
				break;
			}
			month += 1;
			extra_days -= days_of_month[index];
			index += 1;
		}
	}

	// Current month.
	if (extra_days > 0) {
		month += 1;
		date = extra_days;
	} else {
		if (month == 2 && flag == 1) {
			date = 29;
		} else {
			date = days_of_month[month - (month >= 1 ? 1 : 0)];
		}
	}

	hours = extra_time / 3600;
	minutes = (extra_time % 3600) / 60;
	seconds = (extra_time % 3600) % 60;

	int wday = (3 + s_time.tm_mday) % 7;
	if (wday < 0) {
		wday += 7;
	}
	s_time.tm_mday = date;
	s_time.tm_mon = month;
	s_time.tm_year = curr_year;
	s_time.tm_hour = hours;
	s_time.tm_min = minutes;
	s_time.tm_sec = seconds;
	s_time.tm_yday = extra_days;
	s_time.tm_wday = wday;
	s_time.tm_isdst = daylight;

	s_time.tm_gmtoff = 0;
	s_time.tm_zone = "Etc/Gmt";

	return &s_time;
}

int
clock_gettime(clockid_t clock_id, struct timespec *tp)
{
	return __syscall_ret(__syscall2(SYS_clock_gettime, (long)clock_id, (long)tp));
}

time_t
time(time_t *tloc)
{
	struct timespec ts;
	int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
	if (ret < 0) {
		return (time_t)-1;
	}
	if (tloc != NULL) {
		*tloc = ts.tv_sec;
	}
	return 0;
}

int
clock_nanosleep(clockid_t clock_id, int flags, const struct timespec *duration,
                struct timespec *rem)
{
	return __syscall_ret(__syscall4(SYS_clock_nanosleep, clock_id, flags,
	                                (long)duration, (long)rem));
}

int
nanosleep(const struct timespec *duration, struct timespec *rem)
{
	// POSIX.1-2024:
	// "Calling clock_nanosleep() with the value TIMER_ABSTIME not set in the
	// flags argument and with a clock_id of CLOCK_REALTIME is equivalent to
	// calling nanosleep() with the same rqtp and rmtp arguments."
	return clock_nanosleep(CLOCK_REALTIME, 0, duration, rem);
}
