#pragma once

// for compat once we split out these headers more
#include <date.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

// ?
int
uptime(void);
void
echoout(int answer);

// time? more like a nonstandard time(time_t *)
int
date(struct rtcdate *);
