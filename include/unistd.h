#pragma once

int
fork(void);
void
exit(int) __attribute__((noreturn));
int
pipe(int *);
int
exec(char *, char **);
// our exec() is technically execv()
#define execv(x, y) (exec(x, y))
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
char *
sbrk(int);
int
sleep(int);
// needs sys/reboot
void
reboot(int cmd);
void
setuid(int);
