#pragma once

typedef unsigned int uint;
#include <stat.h>
#include <stdio.h>
#include <dirent.h>
#include <date.h>
#include <fcntl.h>
#include <stdarg.h>
#include <reboot.h>

// system calls
int
fork(void);
void
exit(int) __attribute__((noreturn));
int
wait(int *status);
int
pipe(int *);
int
write(int, const void *, int);
int
read(int, void *, int);
int
close(int);
int
kill(int);
int
exec(char *, char **);
// our exec() is technically execv()
#define execv(x, y) (exec(x, y))
int
open(const char *, int);
int
mknod(const char *, short, short);
int
unlink(const char *);
int
fstat(int fd, struct stat *);
int
link(const char *, const char *);
int
mkdir(const char *);
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
int
uptime(void);
int
date(struct rtcdate *);
int
chmod(char *, int mode);
void
reboot(int cmd);
void
echoout(int answer);
void
setuid(int);

// ulib.c
int
stat(const char *, struct stat *);
DIR *
opendir(const char *name);
char *
strcpy(char *, const char *);
void *
memmove(void *, const void *, int);
char *
strchr(const char *, char c);
char *
strcat(char *dst, const char *src);
int
strcmp(const char *, const char *);
void
vfprintf(int, const char *, va_list *argp);
void
fprintf(int, const char *, ...);
void
printf(const char *, ...);
char *
gets(char *, int max);
int
getc(FILE fd);
uint
strlen(const char *);
void *
memset(void *, int, uint);
void *malloc(uint);
void
free(void *);
int
atoi(const char *);
int
atoi_base(const char *, uint base);
