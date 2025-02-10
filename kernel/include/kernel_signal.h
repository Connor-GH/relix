#pragma once
#include <stdint.h>
enum {
	SIGHUP = 1,
	SIGINT = 2,
	SIGQUIT = 3,
	SIGILL = 4,
	SIGTRAP = 5,
	SIGABRT = 6,
	SIGBUS = 7,
	SIGFPE = 8,
	SIGKILL = 9,
	SIGUSR1 = 10,
	SIGSEGV = 11,
	SIGUSR2 = 12,
	SIGPIPE = 13,
	SIGALRM = 14,
	SIGTERM = 15,
	SIGSTKFLT = 16,
	SIGCHLD = 17,
	SIGCONT = 18,
	SIGSTOP = 19,
	SIGTSTP = 20,
	SIGGTTIN = 21,
	SIGTTOU = 22,
	SIGURG = 23,
	SIGXCPU = 24,
	SIGXFSZ = 25,
	SIGVTALRM = 26,
	SIGPROG = 27,
	SIGWINCH = 28,
	SIGIO = 29,
	SIGPWR = 30,
	SIGSYS = 31,
	__SIG_last
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
#define SIG_ERR ((sighandler_t)-1) // Error
#define SIG_DFL ((sighandler_t)-2) // Default action
#define SIG_IGN ((sighandler_t)1) // Ignore

#ifdef __KERNEL__
sighandler_t
kernel_attach_signal(int signum, sighandler_t handler);
#endif
