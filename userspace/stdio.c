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
#include <sys/stat.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

static FILE *open_files[NFILE];
static size_t open_files_index = 0;

static size_t global_idx = 0;
static size_t global_idx_fgetc = 0;

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
	fwrite(stream->write_buffer, 1, stream->write_buffer_index, stream);
	//writev(stream->fd,
	//			 &(const struct iovec){ stream->write_buffer,
	//															stream->write_buffer_index },
	//			 1);
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
	fp->buffer_mode = BUFFER_MODE_BLOCK;
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

FILE *
freopen(const char *restrict pathname, const char *restrict mode,
				FILE *restrict stream)
{
	if (stream && !stream->error && stream->fd != -1) {
		fclose(stream);
	}
	stream = fopen(pathname, mode);
	return stream;
}

int
feof(FILE *stream)
{
	if (stream)
		return stream->eof;
	else
		return -1;
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
	fp->buffer_mode = BUFFER_MODE_BLOCK;
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
		stream->error = true;
		return EOF;
	}
	fflush(stream);
	if (stream->write_buffer != NULL)
		free(stream->write_buffer);
	if (close(stream->fd) < 0) {
		stream->error = true;
		return EOF;
	}
	open_files[stream->static_table_index] = NULL;
	open_files_index--;
	free(stream);

	return 0;
}
int
ferror(FILE *stream)
{
	return stream->error;
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

static void
static_fgetc(FILE *stream, char c, char *buf)
{
	if (global_idx_fgetc < __DIRSIZ - 1)
		buf[global_idx_fgetc++] = getc(stream);
	else
		buf[global_idx_fgetc++] = '\0';
}

char *
fgets(char *buf, int max, FILE *restrict stream)
{
	int i;
	char c;

	for (i = 0; i + 1 < max;) {
		c = getc(stream);
		if (c == EOF) {
			stream->eof = true;
			return NULL;
		}
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
	if (fp &&
		((fp->buffer_mode == BUFFER_MODE_BLOCK &&
		fp->write_buffer_index >= fp->write_buffer_size - 1) ||
	(fp->buffer_mode == BUFFER_MODE_UNBUFFERED) ||
	(fp->buffer_mode == BUFFER_MODE_LINE && c == '\n'))) {
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

int
vfprintf(FILE *restrict stream, const char *restrict fmt, va_list argp)
{
	int ret = sharedlib_vprintf_template(fd_putc, ansi_noop, stream, NULL, fmt, argp,
														NULL, NULL, NULL, false, -1);
	if (stream && stream->stdio_flush)
		flush(stream);
	return ret;
}

int
vsnprintf(char *restrict str, size_t n, const char *restrict fmt, va_list argp)
{
	global_idx = 0;
	return sharedlib_vprintf_template(string_putc, ansi_noop, NULL, str, fmt, argp,
														NULL, NULL, NULL, false, n);
}
int
vsprintf(char *restrict str, const char *restrict fmt, va_list argp)
{
	global_idx = 0;
	return sharedlib_vprintf_template(string_putc, ansi_noop, NULL, str, fmt, argp,
														NULL, NULL, NULL, false, -1);
}
int
snprintf(char *restrict str, size_t n, const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vsnprintf(str, n, fmt, listp);
	va_end(listp);
	return ret;
}

int
sprintf(char *restrict str, const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vsprintf(str, fmt, listp);
	va_end(listp);
	return ret;
}

int
vfscanf(FILE *restrict stream, const char *restrict fmt, va_list argp)
{
	static char static_buffer[__DIRSIZ];
	global_idx_fgetc = 0;
	return sharedlib_vprintf_template(static_fgetc,
														ansi_noop, stream, static_buffer, fmt, argp, NULL, NULL, NULL, false, -1);
}

int
fscanf(FILE *restrict stream, const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vfscanf(stream, fmt, listp);
	va_end(listp);
	return ret;
}

__attribute__((format(printf, 2, 3))) int
fprintf(FILE *restrict stream, const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vfprintf(stream, fmt, listp);
	va_end(listp);
	return ret;
}

__attribute__((format(printf, 1, 2))) int
printf(const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vfprintf(stdout, fmt, listp);
	va_end(listp);
	return ret;
}

int
fputs(const char *restrict s, FILE *restrict stream)
{
	return fprintf(stream, "%s\n", s);
}

int
puts(const char *restrict s)
{
	return fputs(s, stdout);
}

void
setlinebuf(FILE *restrict stream)
{
	stream->buffer_mode = BUFFER_MODE_LINE;
}

void
perror(const char *s)
{
	fprintf(stderr, "%s: %s\n", s, errno_codes[errno]);
}

void
clearerr(FILE *stream)
{
	if (stream) {
		stream->eof = false;
		stream->error = false;
	}
}

const char *
strerror(int err_no)
{
	return errno_codes[err_no];
}
int
remove(const char *pathname)
{
	struct stat st;
	if (stat(pathname, &st) < 0) {
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		return 0;
		// rmdir
	} else if (S_ISREG(st.st_mode)) {
		if (unlink(pathname) < 0)
			return -1;
		return 0;
	}
	return -1;
}

size_t
fwrite(void *ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
	size_t count = size * nmemb;
	int fd = fileno(stream);
	ssize_t ret = write(fd, ptr, count);
	if (ret < 0) {
		stream->error = true;
		return 0;
	}
	stream->file_offset = count;
	return ret / size;
}

size_t
fread(void *ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
	size_t count = size * nmemb;
	int fd = fileno(stream);
	ssize_t ret = read(fd, ptr, count);
	if (ret < 0) {
		stream->error = true;
		return 0;
	}
	stream->file_offset = count;
	return ret / size;
}

long
ftell(FILE *stream)
{
	if (stream) {
		return stream->file_offset;
	} else {
		errno = -EBADF;
		return -1;
	}
}
