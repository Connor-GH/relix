#pragma once
#include <stdint.h>
int
kill(int);

enum {
	SIGINT = 2,
	SIGILL = 4,
	SIGABRT = 6,
	SIGFPE = 8,
	SIGKILL = 9,
	SIGSEGV = 11,
	SIGTERM = 15,
	SIGCHLD = 17,
	SIGSYS = 31,
	__last
};
union sigval {
	int sival_int;
	void *sival_ptr;
};
typedef struct {
	int si_siginfo;
	int si_code;
	uint32_t si_pid;
	uint32_t si_uid;
	void *si_addr;
	int si_status;
	union sigval si_value;
} siginfo_t;

typedef void (*sighandler_t)(int /*, siginfo_t*/);
