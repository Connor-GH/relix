#include "kernel/include/param.h"
#include "lib/print.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/param.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

static FILE *open_files[NFILE];
static size_t open_files_index = 0;

static uint32_t global_idx = 0;

#define WRITE_BUFFER_SIZE 256
void
__init_stdio(void)
{
	FILE *file_stdin = fdopen(0, "r");
	FILE *file_stdout = fdopen(1, "w"); // maybe should be a+?
	FILE *file_stderr = fdopen(2, "w"); // maybe should be a+?
	open_files[0] = file_stdin;
	open_files[1] = file_stdout;
	open_files[2] = file_stderr;
	stdin = file_stdin;
	stdout = file_stdout;
	stderr = file_stderr;
}
void
__fini_stdio(void)
{
	for (size_t i = 0; i < open_files_index; i++) {
		if (open_files[i] != NULL) {
			fclose(open_files[i]);
			open_files[i] = NULL;
		}
	}
}
static int
flush(FILE *stream)
{
	if (stream == NULL) {
		errno = EBADF;
		return -1;
	}
	writev(stream->fd,
				 &(const struct iovec){ stream->write_buffer,
																stream->write_buffer_index },
				 1);
	stream->write_buffer_index = 0;
	return 0;
}

int
fflush(FILE *stream)
{
	if (stream == NULL) {
		fflush(stdout);
		fflush(stderr);
	}
	return flush(stream);
}

int
fileno(FILE *stream)
{
	if (stream != NULL)
		return stream->fd;
	errno = EBADF;
	return -1;
}

static int
string_to_mode(const char *restrict mode)
{
	if (mode == NULL) {
		goto bad_mode;
	}
	int mode_val = -1;
	switch (mode[0]) {
	case 'w': {
		if (mode[1] != '\0') {
			if (mode[1] == '+') {
				mode_val = O_RDWR | O_CREATE /*| O_TRUNC*/;
				break;
			}
		}
		mode_val = O_WRONLY | O_CREATE /*| O_TRUNC*/;
		break;
	}
	case 'r': {
		if (mode[1] != '\0') {
			if (mode[1] == '+') {
				mode_val = O_RDWR;
				break;
			}
		}
		mode_val = O_RDONLY;
		break;
	}
	case 'a': {
		if (mode[1] != '\0') {
			if (mode[1] == '+') {
				mode_val = O_RDWR | O_CREATE | O_APPEND;
				break;
			}
		}
		mode_val = O_WRONLY | O_CREATE | O_APPEND;
		break;
	}
	default:
		goto bad_mode;
	}
	return mode_val;
bad_mode:
	errno = EINVAL;
	return -1;
}

FILE *
fopen(const char *restrict pathname, const char *restrict mode)
{
	FILE *fp = malloc(sizeof(*fp));
	if (fp == NULL)
		return NULL;
	fp->mode = string_to_mode(mode);
	if (fp->mode == -1)
		return NULL;
	fp->fd = open(pathname, fp->mode);
	if (fp->fd == -1)
		return NULL;
	fp->eof = false;
	fp->error = false;
	fp->write_buffer = malloc(WRITE_BUFFER_SIZE);
	if (fp->write_buffer == NULL)
		return NULL;
	fp->write_buffer_size = WRITE_BUFFER_SIZE;
	fp->write_buffer_index = 0;
	fp->stdio_flush = true;
	if (open_files_index < NFILE) {
		fp->static_table_index = open_files_index;
		open_files[open_files_index++] = fp;
	} else {
		for (size_t i = 0; i < NFILE; i++) {
			if (open_files[i] == NULL) {
				fp->static_table_index = i;
				open_files[i] = fp;
			}
		}
		return NULL;
	}
	return fp;
}

// Mandated by POSIX
FILE *
fdopen(int fd, const char *restrict mode)
{
	if (fd == -1) {
		errno = EBADF;
		return NULL;
	}
	FILE *fp = malloc(sizeof(*fp));
	if (fp == NULL)
		return NULL;
	fp->mode = string_to_mode(mode);
	if (fp->mode == -1)
		return NULL;
	fp->fd = fd;
	fp->eof = false;
	fp->error = false;
	fp->write_buffer = malloc(WRITE_BUFFER_SIZE);
	if (fp->write_buffer == NULL)
		return NULL;
	fp->write_buffer_size = WRITE_BUFFER_SIZE;
	fp->write_buffer_index = 0;
	fp->stdio_flush = true;
	if (open_files_index < NFILE) {
		fp->static_table_index = open_files_index;
		open_files[open_files_index++] = fp;
	} else {
		for (size_t i = 0; i < NFILE; i++) {
			if (open_files[i] == NULL) {
				fp->static_table_index = i;
				open_files[i] = fp;
			}
		}
		return NULL;
	}
	return fp;
}

int
fclose(FILE *stream)
{
	if (stream == NULL) {
		errno = EBADF;
		return EOF;
	}
	fflush(stream);
	if (stream->write_buffer != NULL)
		free(stream->write_buffer);
	if (close(stream->fd) < 0)
		return EOF;
	open_files[stream->static_table_index] = NULL;
	open_files_index--;
	free(stream);

	return 0;
}

int
getc(FILE *stream)
{
	int c;
	if (read(fileno(stream), &c, 1) != 1) {
		return EOF;
	}
	fflush(stream);
	return c;
}

char *
fgets(char *buf, int max, FILE *restrict stream)
{
	int i;
	char c;

	for (i = 0; i + 1 < max;) {
		c = getc(stream);
		if (c == EOF)
			return NULL;
		buf[i++] = c;
		if (c == '\n' || c == '\r')
			break;
	}
	buf[i] = '\0';
	return buf;
}

/* We don't care about what's passed in buf */
// This is where the buffered IO happens.
// It functions for both characters and pixels
// (in the format described in libgui)
static void
fd_putc(FILE *fp, char c, char *__attribute__((unused)) buf)
{
	if (fp && fp->write_buffer_index >= fp->write_buffer_size - 1) {
		flush(fp);
	} else {
		fp->write_buffer[fp->write_buffer_index++] = c;
	}
}
static void
string_putc(FILE *__attribute__((unused)) fp, char c, char *buf)
{
	buf[global_idx] = c;
	global_idx++;
}
int
fputc(int c, FILE *stream)
{
	fd_putc(stream, c, NULL);
	return c;
}
int
putchar(int c)
{
	return putc(c, stdout);
}

static size_t
ansi_noop(const char *s)
{
	return 1;
}
void
vfprintf(FILE *restrict stream, const char *fmt, va_list *argp)
{
	sharedlib_vprintf_template(fd_putc, ansi_noop, stream, NULL, fmt, argp,
														NULL, NULL, NULL, false);
	if (stream && stream->stdio_flush)
		flush(stream);
}

void
vsprintf(char *restrict str, const char *restrict fmt, va_list *argp)
{
	global_idx = 0;
	sharedlib_vprintf_template(string_putc, ansi_noop, NULL, str, fmt, argp,
														NULL, NULL, NULL, false);
}
void
sprintf(char *restrict str, const char *restrict fmt, ...)
{
	va_list listp;
	va_start(listp, fmt);
	vsprintf(str, fmt, &listp);
	va_end(listp);
}

__attribute__((format(printf, 2, 3))) void
fprintf(FILE *restrict stream, const char *fmt, ...)
{
	va_list listp;
	va_start(listp, fmt);
	vfprintf(stream, fmt, &listp);
	va_end(listp);
}

__attribute__((format(printf, 1, 2))) void
printf(const char *fmt, ...)
{
	va_list listp;
	va_start(listp, fmt);
	vfprintf(stdout, fmt, &listp);
	va_end(listp);
}

void
perror(const char *s)
{
	fprintf(stderr, "%s: %s\n", s, errno_codes[errno]);
}

const char *
strerror(int err_no)
{
	return errno_codes[err_no];
}
