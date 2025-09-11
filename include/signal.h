#pragma once
#include <bits/size_t.h>
#include <bits/stdint.h>
#include <bits/struct_timespec.h>
#include <bits/types.h>

typedef __uid_t uid_t;
typedef __pid_t pid_t;
typedef __size_t size_t;

#define SIGHUP 1
#define SIGINT 2
#define SIGQUIT 3
#define SIGILL 4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS 7
#define SIGFPE 8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG 23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO 29
#define SIGPOLL SIGIO
#define SIGPWR 30
#define SIGSYS 31

#define NSIG 31
#define __SIG_BIT(sig) (1U << (sig))

union sigval {
	int sival_int;
	void *sival_ptr;
};

typedef struct {
	int si_signo; /* Signal number */
	int si_code; /* Signal code */
	pid_t si_pid; /* Sending process ID */
	uid_t si_uid; /* Real user ID of sending process */
	void *si_addr; /* Memory location which caused fault */
	int si_status; /* Exit value or signal */
	union sigval si_value; /* Signal value */
} siginfo_t;

typedef void (*sighandler_t)(int);
typedef __uint32_t sigset_t;
struct sigaction {
	union {
		void (*sa_handler)(int);
		void (*sa_sigaction)(int, siginfo_t *, void *);
	};
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

#define SIG_ERR ((sighandler_t) - 1) // Error
#define SIG_DFL ((sighandler_t)0) // Default action
#define SIG_IGN ((sighandler_t)1) // Ignore

#define SIG_SETMASK 1
#define SIG_BLOCK 2
#define SIG_UNBLOCK 3

#define SA_NOCLDSTOP 1
#define SA_ONSTACK 2
#define SA_RESETHAND 3
#define SA_RESTART 4
#define SA_SIGINFO 5
#define SA_NOCLDWAIT 6
#define SA_NODEFER 7
#define SS_ONSTACK 8
#define SS_DISABLE 9
#define MINSIGSTKSZ 10
#define SIGSTKSZ 11
int kill(pid_t pid, int signal);
int killpg(pid_t pgid, int sig);
sighandler_t signal(int signum, sighandler_t handler);
// Seems strange, but everyone defines it this way.
// Int modifications, at least on x86, are an atomic transaction.
typedef int sig_atomic_t;
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
int sigismember(const sigset_t *set, int signum);

int sigprocmask(int how, const sigset_t *restrict set,
                sigset_t *restrict oldset);
int raise(int sig);
int sigsuspend(const sigset_t *mask);
int sigaction(int signum, const struct sigaction *restrict act,
              struct sigaction *restrict oldact);
void psignal(int sig, const char *s);
