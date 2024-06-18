#pragma once
#include <stdarg.h>
struct _IO_FILE {
	/* TODO */
};

typedef int FILE;

#define __file_stdin 0
#define __file_stdout 1
#define __file_stderr 2

/* TODO turn this into FILE * when we get proper stdio */
#define stdin ((FILE)__file_stdin)
#define stdout ((FILE)__file_stdout)
#define stderr ((FILE)__file_stderr)

#define EOF (-1)
#define DIRSIZ 254
#define FILENAME_MAX DIRSIZ

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
