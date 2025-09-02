#pragma once

#ifndef _GNU_SOURCE
#error "This header is not POSIX. Add `#define _GNU_SOURCE'."
#else
extern char *optarg;
extern int optind, opterr, optopt;
int getopt(int argc, char *const argv[], const char *optstring);
#endif
