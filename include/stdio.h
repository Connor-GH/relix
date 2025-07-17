#pragma once

// mkfs defines this when building.
#ifndef USE_HOST_TOOLS

#if defined(__USER__)
#include <bits/__DIRSIZ.h>
#include <bits/__NULL.h>
#include <bits/size_t.h>
#include <bits/types.h>
#include <bits/va_list.h>

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

#define EOF (-1)
#define FILENAME_MAX __DIRSIZ

int vfprintf(FILE *restrict, const char *restrict fmt, va_list argp);
__attribute__((format(printf, 2, 3))) int
fprintf(FILE *restrict, const char *restrict fmt, ...);
int vprintf(const char *restrict fmt, va_list argp);
__attribute__((format(printf, 1, 2))) int printf(const char *restrict fmt, ...);
__attribute__((format(printf, 2, 3))) int
dprintf(int fd, const char *restrict fmt, ...);
__attribute__((format(printf, 3, 4))) int
snprintf(char *restrict str, size_t n, const char *restrict fmt, ...);
int vsnprintf(char *restrict str, size_t n, const char *restrict fmt,
              va_list argp);
int vsprintf(char *restrict str, const char *restrict fmt, va_list argp);
__attribute__((format(printf, 2, 3))) int
sprintf(char *restrict str, const char *restrict fmt, ...);
char *fgets(char *restrict buf, int max, FILE *restrict steam);
int fputs(const char *restrict s, FILE *restrict stream);
int puts(const char *s);
void setlinebuf(FILE *restrict stream);
int setvbuf(FILE *restrict stream, char *restrict buf, int modes, size_t n);
void setbuf(FILE *restrict stream, char *restrict buf);
int getc(FILE *stream);
int getchar(void);
int ungetc(int c, FILE *stream);
int fileno(FILE *stream);
void perror(const char *s);
int fputc(int c, FILE *stream);
int putchar(int c);
int fflush(FILE *stream);
FILE *fdopen(int fd, const char *restrict mode);
FILE *fopen(const char *restrict pathname, const char *restrict mode);
FILE *freopen(const char *restrict pathname, const char *restrict mode,
              FILE *restrict stream);
int fscanf(FILE *restrict stream, const char *restrict fmt, ...);
int sscanf(const char *restrict str, const char *restrict fmt, ...);
int feof(FILE *stream);
int fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *restrict stream);
size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *restrict stream);
int fseek(FILE *stream, long offset, int whence);
int ferror(FILE *stream);
void clearerr(FILE *stream);
void rewind(FILE *stream);

int remove(const char *pathname);
int rename(const char *oldpath, const char *newpath);
long ftell(FILE *stream);
#define putc(c, stream) fputc(c, stream)
#endif
#endif /* USE_HOST_TOOLS */
