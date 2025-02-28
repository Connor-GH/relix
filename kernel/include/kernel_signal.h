#pragma once
#include <stdint.h>
#include "types.h"
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
	SIGTTIN = 21,
	SIGTTOU = 22,
	SIGURG = 23,
	SIGXCPU = 24,
	SIGXFSZ = 25,
	SIGVTALRM = 26,
	SIGPROF = 27,
	SIGWINCH = 28,
	SIGIO = 29,
	SIGPOLL = SIGIO,
	SIGPWR = 30,
	SIGSYS = 31,
	__SIG_last
};
#define NSIG __SIG_last - 1

union sigval {
	int sival_int;
	void *sival_ptr;
};

typedef struct {
  int si_signo;  /* Signal number */
  int si_code;   /* Signal code */
  pid_t si_pid;    /* Sending process ID */
  uid_t si_uid;    /* Real user ID of sending process */
  void *si_addr;   /* Memory location which caused fault */
  int si_status; /* Exit value or signal */
  union sigval si_value;  /* Signal value */
} siginfo_t;

typedef void (*sighandler_t)(int);
typedef int sigset_t;
struct sigaction {
	union {
		void (*sa_handler)(int);
		void (*sa_sigaction)(int, siginfo_t *, void *);
	};
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

#define SIG_ERR ((sighandler_t)-1) // Error
#define SIG_DFL ((sighandler_t)-2) // Default action
#define SIG_IGN ((sighandler_t)1) // Ignore

#define SIG_SETMASK 0x1

#ifdef __KERNEL__
sighandler_t
kernel_attach_signal(int signum, sighandler_t handler);
#endif
