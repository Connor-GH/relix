#pragma once
#include <sys/types.h>
pid_t
wait(int *status);
struct rusage;
pid_t
wait3(int *status, int options,
		struct rusage *rusage);
#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFEXITED(status) (1)
#define WTERMSIG(status) (1)

// Options.
#define WNOHANG 0x1
