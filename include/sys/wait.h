#pragma once
#include <signal.h>
#include <sys/types.h>
pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);
#if defined(_BSD_SOURCE) || defined(_XOPEN_SOURCE) || defined(_DEFAULT_SOURCE)
#include <sys/resource.h>
pid_t wait4(pid_t pid, int *status, int options, struct rusage *rusage);
pid_t wait3(int *status, int options, struct rusage *rusage);
#endif

// Our pid_t for wait is laid out like this:
// 0b00000000000000000000000000000000
//   [       r      ][   R  ][   S  ]
//
// Legend:
// S: signal returned
// R: return value
// r: reserved

#define WEXITSTATUS(status) (((status) & 0xff00) >> 8)
#define WIFEXITED(status) (WEXITSTATUS(status) == 0)

#define WTERMSIG(status) (((status) & 0x00ff))
#define WIFSIGNALED(status) (WTERMSIG(status) != 0)
#define WIFSTOPPED(status) (WTERMSIG(status) == SIGSTOP)
#define WSTOPSIG(status) (SIGSTOP)
#define WIFCONTINUED(status) (WTERMSIG(status) == SIGCONT)

#define WEXITED 0x2
#define WNOWAIT 0x4
#define WSTOPPED 0x8
// Options.
#define WNOHANG 0x1
#define WCONTINUED 0x2
#define WUNTRACED 0x4

enum idtype_t { P_ALL, P_PGID, P_PID };
