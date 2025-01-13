#pragma once

#include <stdarg.h>
#include "kernel/include/fs.h"
#if defined(FILE_STRUCT_DONE) && FILE_STRUCT_DONE
struct _IO_FILE {
	/* TODO */
};
#endif

// mkfs defines this when building.
#ifndef USE_HOST_TOOLS

typedef int FILE;

/* TODO turn this into FILE * when we get proper stdio */
#define stdin ((FILE *)&(FILE){ 0 })
#define stdout ((FILE *)&(FILE){ 1 })
#define stderr ((FILE *)&(FILE){ 2 })

#define EOF (-1)
#define FILENAME_MAX __DIRSIZ

void
vfprintf(FILE *restrict, const char *, va_list *argp);
__attribute__((format(printf, 2, 3))) void
fprintf(FILE *restrict, const char *, ...);
__attribute__((format(printf, 1, 2))) void
printf(const char *, ...);
void
vsprintf(char *restrict str, const char *restrict fmt, va_list *argp);
void
sprintf(char *restrict str, const char *restrict fmt, ...);
char *
fgets(char *buf, int max, FILE *restrict steam);
int
getc(FILE *stream);
int
fileno(FILE *stream);
void
perror(const char *s);
int
fputc(int c, FILE *stream);
int
putchar(int c);
#define putc(c, stream) fputc(c, stream)
#endif /* USE_HOST_TOOLS */
