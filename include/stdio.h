#pragma once

// mkfs defines this when building.
#ifndef USE_HOST_TOOLS

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/include/fs.h"

#if defined(__ONLY_SHARE_FILE_IMPL) || defined(__USER__)
#include <sys/types.h>
struct _IO_FILE {
	char *write_buffer;
	size_t write_buffer_size;
	size_t write_buffer_index;
	size_t static_table_index;
	int fd;
	int mode;
	int buffer_mode;
	off_t file_offset;
	bool eof;
	bool error;
	bool stdio_flush;
};
#define BUFFER_MODE_LINE 0x1
#define BUFFER_MODE_BLOCK 0x2
#define BUFFER_MODE_UNBUFFERED 0x3
#define _IONBF BUFFER_MODE_UNBUFFERED
#define _IOLBF BUFFER_MODE_LINE
#define _IOFBF BUFFER_MODE_BLOCK

typedef struct _IO_FILE FILE;
#endif
#if defined(__USER__)
#include <unistd.h>
#define BUFSIZ 512
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define EOF (-1)
#define FILENAME_MAX __DIRSIZ
#define NAME_MAX FILENAME_MAX

int
vfprintf(FILE *restrict, const char *restrict fmt, va_list argp);
__attribute__((format(printf, 2, 3))) int
fprintf(FILE *restrict, const char *restrict fmt, ...);
__attribute__((format(printf, 1, 2))) int
printf(const char *restrict fmt, ...);
__attribute__((format(printf, 2, 3))) int
dprintf(int fd, const char *restrict fmt, ...);
int
snprintf(char *restrict str, size_t n, const char *restrict fmt, ...);
int
vsnprintf(char *restrict str, size_t n, const char *restrict fmt, va_list argp);
int
vsprintf(char *restrict str, const char *restrict fmt, va_list argp);
int
sprintf(char *restrict str, const char *restrict fmt, ...);
char *
fgets(char *restrict buf, int max, FILE *restrict steam);
int
fputs(const char *restrict s, FILE *restrict stream);
int
puts(const char *s);
void
setlinebuf(FILE *restrict stream);
int
setvbuf(FILE *restrict stream, char *restrict buf, int modes, size_t n);
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
FILE *
freopen(const char *restrict pathname, const char *restrict mode,
				FILE *restrict stream);
int
fscanf(FILE *restrict stream, const char *restrict fmt, ...);
int
sscanf(const char *restrict str, const char *restrict fmt, ...);
int
feof(FILE *stream);
int
fclose(FILE *stream);
size_t
fread(void *ptr, size_t size, size_t nmemb, FILE *restrict stream);
size_t
fwrite(void *ptr, size_t size, size_t nmemb, FILE *restrict stream);
int
fseek(FILE *stream, long offset, int whence);
int
ferror(FILE *stream);
void
clearerr(FILE *stream);
int
remove(const char *pathname);
int
rename(const char *oldpath, const char *newpath);
long
ftell(FILE *stream);
#define putc(c, stream) fputc(c, stream)
#endif
#endif /* USE_HOST_TOOLS */
