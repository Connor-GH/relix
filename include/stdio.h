#pragma once

// mkfs defines this when building.
#ifndef USE_HOST_TOOLS

#if defined(__USER__)
#include <bits/__NULL.h>
#include <bits/seek_constants.h>
#include <bits/size_t.h>
#include <bits/types.h>
#include <bits/va_list.h>

#ifndef __NONNULL
#define __NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#endif
#ifndef __PRINTF_LIKE
#define __PRINTF_LIKE(...) __attribute__((__format__(__printf__, __VA_ARGS__)))
#endif
#ifndef __SCANF_LIKE
#define __SCANF_LIKE(...) __attribute__((__format__(__scanf__, __VA_ARGS__)))
#endif

typedef __va_list va_list;
typedef __off_t off_t;
typedef __size_t size_t;
typedef __ssize_t ssize_t;
typedef __off_t fpos_t;

struct _IO_FILE {
	char *write_buffer;
	size_t write_buffer_size;
	size_t write_buffer_index;
	size_t static_table_index;
	int previous_char;
	int fd;
	int mode;
	int flags;
	int buffer_mode;
	_Bool eof;
	_Bool error;
	_Bool stdio_flush;
};

#define NULL __NULL
#define _IOLBF 0x1
#define _IOFBF 0x2
#define _IONBF 0x3

typedef struct _IO_FILE FILE;
#define BUFSIZ 512
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

#define stdin stdin
#define stdout stdout
#define stderr stderr

#define EOF (-1)

int vfprintf(FILE *restrict, const char *restrict fmt, va_list argp)
	__NONNULL(1);
int fprintf(FILE *restrict, const char *restrict fmt, ...) __PRINTF_LIKE(2, 3)
	__NONNULL(1);
int vprintf(const char *restrict fmt, va_list argp);
int printf(const char *restrict fmt, ...) __PRINTF_LIKE(1, 2);
int dprintf(int fd, const char *restrict fmt, ...) __PRINTF_LIKE(2, 3);
int snprintf(char *restrict str, size_t n, const char *restrict fmt, ...)
	__PRINTF_LIKE(3, 4);
int vsnprintf(char *restrict str, size_t n, const char *restrict fmt,
              va_list argp);
int vsprintf(char *restrict str, const char *restrict fmt, va_list argp);
int sprintf(char *restrict str, const char *restrict fmt, ...)
	__PRINTF_LIKE(2, 3);
char *fgets(char *restrict buf, int max, FILE *restrict stream) __NONNULL(3);
int fputs(const char *restrict s, FILE *restrict stream) __NONNULL(2);
int puts(const char *s);
void setlinebuf(FILE *restrict stream) __NONNULL(1);
int setvbuf(FILE *restrict stream, char *restrict buf, int modes, size_t n)
	__NONNULL(1);
void setbuf(FILE *restrict stream, char *restrict buf) __NONNULL(1);

int fgetc(FILE *stream) __NONNULL(1);
int getc(FILE *stream) __NONNULL(1);
int getchar(void);

int ungetc(int c, FILE *stream) __NONNULL(2);

int fputc(int c, FILE *stream) __NONNULL(2);
int putc(int c, FILE *stream) __NONNULL(2);
int putchar(int c);

int fileno(FILE *stream) __NONNULL(1);
void perror(const char *s);
// stream can be NULL.
int fflush(FILE *stream);
FILE *fdopen(int fd, const char *restrict mode);
FILE *fopen(const char *restrict pathname, const char *restrict mode);
FILE *freopen(const char *restrict pathname, const char *restrict mode,
              FILE *restrict stream) __NONNULL(3);
int fscanf(FILE *restrict stream, const char *restrict fmt, ...)
	__SCANF_LIKE(2, 3) __NONNULL(1);
int sscanf(const char *restrict str, const char *restrict fmt, ...)
	__SCANF_LIKE(2, 3);
int feof(FILE *stream) __NONNULL(1);
int fclose(FILE *stream) __NONNULL(1);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *restrict stream)
	__NONNULL(4);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *restrict stream)
	__NONNULL(4);
int fseek(FILE *stream, long offset, int whence) __NONNULL(1);
int ferror(FILE *stream) __NONNULL(1);
void clearerr(FILE *stream) __NONNULL(1);
void rewind(FILE *stream) __NONNULL(1);

int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);
long ftell(FILE *stream) __NONNULL(1);

#endif
#endif /* USE_HOST_TOOLS */
