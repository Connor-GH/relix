#pragma once
#include <stdarg.h>
#if defined(FILE_STRUCT_DONE) && FILE_STRUCT_DONE
struct _IO_FILE {
	/* TODO */
};
#endif

typedef int FILE;

#define __file_stdin 0
#define __file_stdout 1
#define __file_stderr 2

/* TODO turn this into FILE * when we get proper stdio */
#define stdin ((FILE)__file_stdin)
#define stdout ((FILE)__file_stdout)
#define stderr ((FILE)__file_stderr)

#define EOF (-1)
#define DIRSIZ 14
#define FILENAME_MAX DIRSIZ

void
vfprintf(int, const char *, va_list *argp);
__attribute__((format(printf, 2, 3)))
void
fprintf(int, const char *, ...);
__attribute__((format(printf, 1, 2)))
void
printf(const char *, ...);
void
vsprintf(char *restrict str, const char *restrict fmt, va_list *argp);
void
sprintf(char *restrict str, const char *restrict fmt, ...);
char *
gets(char *, int max);
int
getc(FILE fd);
void
perror(const char *s);
