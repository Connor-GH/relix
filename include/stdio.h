#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/include/fs.h"

// mkfs defines this when building.
#ifndef USE_HOST_TOOLS
struct _IO_FILE {
	char *write_buffer;
	size_t write_buffer_size;
	size_t write_buffer_index;
	size_t static_table_index;
	int fd;
	int mode;
	bool eof;
	bool error;
};

#include <stat.h>

typedef struct _IO_FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

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
int
fflush(FILE *stream);
FILE *
fdopen(int fd, const char *restrict mode);
FILE *
fopen(const char *restrict pathname, const char *restrict mode);
int
fclose(FILE *stream);
#define putc(c, stream) fputc(c, stream)
#endif /* USE_HOST_TOOLS */
