/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include "libc_syscalls.h"
#include "printf.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

int errno;

static FILE *open_files[OPEN_MAX];
static size_t open_files_index = 0;

static size_t global_idx = 0;
static size_t global_idx_fgetc = 0;

void
__init_stdio(void)
{
	FILE *file_stdin = fdopen(STDIN_FILENO, "r");
	FILE *file_stdout = fdopen(STDOUT_FILENO, "w"); // maybe should be a+?
	FILE *file_stderr = fdopen(STDERR_FILENO, "w"); // maybe should be a+?
	/* stdin, stdout are line buffered by default. */
	if (file_stdin == NULL || file_stderr == NULL || file_stdout == NULL) {
		raise(SIGSEGV);
	}
	setvbuf(file_stdin, NULL, _IOLBF, 0);
	setvbuf(file_stdout, NULL, _IOLBF, 0);
	/* stderr is unbuffered by default. */
	setvbuf(file_stderr, NULL, _IONBF, 0);
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

int
rename(const char *oldpath, const char *newpath)
{
	return __syscall_ret(__syscall4(SYS_renameat, (long)AT_FDCWD, (long)oldpath,
	                                (long)AT_FDCWD, (long)newpath));
}

static int
flush(FILE *stream)
{
	if (__unlikely(stream == NULL)) {
		errno = EBADF;
		return -1;
	}

	// write(stream->fd, stream->write_buffer, stream->write_buffer_index);
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
	if (__unlikely(stream == NULL)) {
		for (size_t i = 0; i < open_files_index; i++) {
			(void)flush(open_files[i]);
		}
	}
	return flush(stream);
}

int
fileno(FILE *stream)
{
	return stream->fd;
}

static int
string_to_flags(const char *restrict mode)
{
	int flags;
	if (strchr(mode, '+')) {
		flags = O_RDWR;
	} else if (mode[0] == 'r') {
		flags = O_RDONLY;
	} else {
		flags = O_WRONLY;
	}
	if (strchr(mode, 'x')) {
		flags |= O_EXCL;
	}
	if (strchr(mode, 'e')) {
		flags |= O_CLOEXEC;
	}
	if (mode[0] != 'r') {
		flags |= O_CREAT;
	}
	if (mode[0] == 'w') {
		flags |= O_TRUNC;
	}
	if (mode[0] == 'a') {
		flags |= O_APPEND;
	}
	return flags;
}

// The "mode" is the "file open mode", not the "permissions mode".
FILE *
fopen(const char *restrict pathname, const char *restrict mode)
{
	const mode_t file_mode = S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH | S_IWUSR |
	                         S_IRUSR;
	// First character of mode must be r, w, or a.
	if (strchr("rwa", mode[0]) == NULL) {
		errno = EINVAL;
		return NULL;
	}
	int flags = string_to_flags(mode);
	int fd = open(pathname, flags, file_mode);
	if (fd == -1) {
		return NULL;
	}
	return fdopen(fd, mode);
}

FILE *
freopen(const char *restrict pathname, const char *restrict mode,
        FILE *restrict stream)
{
	int flags = string_to_flags(mode);

	(void)fflush(stream);

	if (pathname != NULL) {
		(void)close(stream->fd);
	}

	clearerr(stream);

	if (pathname == NULL) {
		fcntl(stream->fd, F_SETFL, flags);
		stream->flags = flags;
	}
	stream->fd = open(pathname, flags, stream->mode);
	stream->write_buffer_index = 0;
	return stream;
}

int
feof(FILE *stream)
{
	return stream->eof;
}

// Mandated by POSIX
FILE *
fdopen(int fd, const char *restrict mode)
{
	if (fd == -1) {
		errno = EBADF;
		return NULL;
	}
	FILE *fp = malloc(sizeof(FILE));
	if (__unlikely(fp == NULL)) {
		return NULL;
	}
	fp->flags = string_to_flags(mode);
	if (fp->flags == -1) {
		free(fp);
		return NULL;
	}
	fp->mode = S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH | S_IWUSR | S_IRUSR;
	fp->fd = fd;
	fp->eof = false;
	fp->error = false;
	fp->write_buffer = malloc(BUFSIZ);
	if (__unlikely(fp->write_buffer == NULL)) {
		free(fp);
		return NULL;
	}
	fp->write_buffer_size = BUFSIZ;
	fp->write_buffer_index = 0;
	fp->stdio_flush = true;
	fp->previous_char = EOF;
	fp->buffer_mode = _IOFBF;
	if (open_files_index < OPEN_MAX) {
		fp->static_table_index = open_files_index;
		open_files[open_files_index++] = fp;
	} else {
		for (size_t i = 0; i < OPEN_MAX; i++) {
			if (open_files[i] == NULL) {
				fp->static_table_index = i;
				open_files[i] = fp;
				return fp;
			}
		}
		free(fp->write_buffer);
		free(fp);
		return NULL;
	}
	return fp;
}

int
fclose(FILE *stream)
{
	fflush(stream);
	if (stream->write_buffer != NULL) {
		free(stream->write_buffer);
	}
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
ungetc(int c, FILE *stream)
{
	return stream->previous_char;
}

int
fgetc(FILE *stream)
{
	int c;
	if (read(fileno(stream), &c, 1) != 1) {
		return EOF;
	}
	stream->previous_char = c;
	return c;
}

int
getc(FILE *stream)
{
	return fgetc(stream);
}

int
getchar(void)
{
	return getc(stdin);
}

static void
static_fgetc(FILE *stream, char c, char *buf)
{
	if (global_idx_fgetc < BUFSIZ - 1) {
		buf[global_idx_fgetc++] = getc(stream);
	} else {
		buf[global_idx_fgetc++] = '\0';
	}
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
		if (c == '\n' || c == '\r') {
			break;
		}
	}
	buf[i] = '\0';
	return buf;
}

/* We don't care about what's passed in buf */
// This is where the buffered IO happens.
__NONNULL(1)
static void
fd_putc(FILE *fp, char c, char *__attribute__((unused)) buf)

{
	fp->write_buffer[fp->write_buffer_index++] = c;
	if (((fp->buffer_mode == _IOFBF &&
	      fp->write_buffer_index >= fp->write_buffer_size - 1) ||
	     (fp->buffer_mode == _IONBF) ||
	     (fp->buffer_mode == _IOLBF && c == '\n'))) {
		flush(fp);
	}
}

__NONNULL(3)
static void
string_putc(__attribute__((unused)) FILE *fp, char c, char *buf)
{
	buf[global_idx] = c;
	global_idx++;
}

static void
null_string_putc(__attribute__((unused)) FILE *fp,
                 __attribute__((unused)) char c,
                 __attribute__((unused)) char *buf)
{
	global_idx++;
}

int
fputc(int c, FILE *stream)
{
	fd_putc(stream, c, NULL);
	return c;
}

int
putc(int c, FILE *stream)
{
	return fputc(c, stream);
}

int
putchar(int c)
{
	return putc(c, stdout);
}

int
vfprintf(FILE *restrict stream, const char *restrict fmt, va_list argp)
{
	int ret =
		__libc_vprintf_template(fd_putc, NULL, stream, NULL, fmt, argp, SIZE_MAX);
	if (stream->stdio_flush) {
		flush(stream);
	}
	return ret;
}

int
vsnprintf(char *restrict str, size_t n, const char *restrict fmt, va_list argp)
{
	global_idx = 0;
	int idx =
		__libc_vprintf_template(str == NULL ? null_string_putc : string_putc, NULL,
	                          NULL, str, fmt, argp, n);
	if (str != NULL) {
		str[idx] = '\0';
	}
	return idx;
}
int
vsprintf(char *restrict str, const char *restrict fmt, va_list argp)
{
	global_idx = 0;
	int ret =
		__libc_vprintf_template(string_putc, NULL, NULL, str, fmt, argp, SIZE_MAX);
	if (str != NULL) {
		str[ret] = '\0';
	}
	return ret;
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
	if (str != NULL) {
		str[ret] = '\0';
	}
	return ret;
}

int
vfscanf(FILE *restrict stream, const char *restrict fmt, va_list argp)
{
	static char static_buffer[BUFSIZ];
	global_idx_fgetc = 0;
	return __libc_vprintf_template(static_fgetc, NULL, stream, static_buffer, fmt,
	                               argp, -1);
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

/* This is only a temporary incomplete implementation. */
int
sscanf(const char *restrict str, const char *restrict fmt, ...)
{
	int state = 0;
	int count = 0;
	size_t i = 0;
	size_t j = 0;
	size_t format_size = 0;
	va_list listp;
	va_start(listp, fmt);

	while (str[i] != '\0' && fmt[j] != '\0') {
		if (fmt[j] == '%') {
			count++;
			state = '%';
			format_size++;
			goto skip_str_forward;
		} else if (state == '%') {
			switch (fmt[j]) {
			case 'i':
			case 'd': {
				format_size++;
				char *cont;
				int d = strtol(str + i, &cont, 10);
				*va_arg(listp, int *) = d;
				i += cont - (str + i) - format_size;
				format_size = 0;
				state = 0;
				break;
			}
			default: {
				if (count > 0) {
					count--;
				}
				state = 0;
				break;
			}
			}

		} else {
			// ??
		}
		i++;
skip_str_forward:
		j++;
	}
	va_end(listp);
	return count;
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

int
dprintf(int fd, const char *restrict fmt, ...)
{
	FILE *fp = fdopen(fd, "w");
	if (fp == NULL) {
		return -1;
	}
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vfprintf(fp, fmt, listp);
	va_end(listp);
	fclose(fp);
	return ret;
}

int
vprintf(const char *restrict fmt, va_list argp)
{
	return vfprintf(stdout, fmt, argp);
}

int
printf(const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vprintf(fmt, listp);
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

int
setvbuf(FILE *restrict stream, char *restrict buf, int modes, size_t n)
{
	if (__likely(buf != NULL)) {
		fflush(stream);
		stream->write_buffer = buf;
		stream->write_buffer_size = n;
	}
	if (__unlikely(!(modes == _IOFBF || modes == _IOLBF || modes == _IONBF))) {
		errno = EINVAL;
		return -1;
	}
	stream->buffer_mode = modes;
	return 0;
}

void
setlinebuf(FILE *restrict stream)
{
	setvbuf(stream, NULL, _IOLBF, 0);
}

void
setbuf(FILE *restrict stream, char *restrict buf)
{
	setvbuf(stream, buf, buf ? _IOFBF : _IONBF, BUFSIZ);
}

void
perror(const char *s)
{
	fprintf(stderr, "%s: %s\n", s, strerror(errno));
}

void
clearerr(FILE *stream)
{
	stream->eof = false;
	stream->error = false;
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
		if (unlink(pathname) < 0) {
			return -1;
		}
		return 0;
	}
	return -1;
}

size_t
fwrite(const void *ptr, size_t size, size_t nmemb, FILE *restrict stream)
{
	size_t count = size * nmemb;
	int fd = fileno(stream);
	ssize_t ret = write(fd, ptr, count);
	if (ret < 0) {
		stream->error = true;
		return 0;
	}
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
	return ret / size;
}

long
ftell(FILE *stream)
{
	return lseek(stream->fd, 0L, SEEK_CUR);
}

void
rewind(FILE *stream)
{
	(void)fseek(stream, 0L, SEEK_SET);
	clearerr(stream);
}

int
fseek(FILE *stream, long offset, int whence)
{
	clearerr(stream);
	return lseek(stream->fd, offset, whence);
}
