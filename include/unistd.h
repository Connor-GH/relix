#pragma once

int
fork(void) __attribute__((returns_twice));
void
exit(int) __attribute__((noreturn));
int
pipe(int *);
int
exec(char *, char **);
// our exec() is technically execv()
static inline int
execv(char *prog, char **argv)
{
	return exec(prog, argv);
}
int
write(int, const void *, int);
int
read(int, void *, int);
int
close(int);
int
unlink(const char *);
int
link(const char *, const char *);
int
chdir(const char *);
int
dup(int);
int
getpid(void);
void *
sbrk(int);
int
sleep(int);
// needs sys/reboot
int
reboot(int cmd);
int
setuid(int);
extern char *optarg;
extern int optind, opterr, optopt;
int
getopt(int argc, char *const argv[], const char *optstring);
