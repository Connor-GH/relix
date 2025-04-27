#include "kernel/include/param.h"
#include "libc_syscalls.h"
#include <sys/syscall.h>
#include "printf.h"
#include "stat.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <gui.h>

FILE *stdin;
FILE *stdout;
FILE *stderr;

static FILE *open_files[NFILE];
static size_t open_files_index = 0;

static size_t global_idx = 0;
static size_t global_idx_fgetc = 0;

#define WRITE_BUFFER_SIZE 256
// This runs on all processes besides init.
void
__init_stdio(void)
{
	FILE *file_stdin = fdopen(0, "r");
	FILE *file_stdout = fdopen(1, "w"); // maybe should be a+?
	FILE *file_stderr = fdopen(2, "w"); // maybe should be a+?
	/* stdin, stdout are line buffered by default. */
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
	return __syscall2(SYS_rename, (long)oldpath, (long)newpath);
}

static int
flush(FILE *stream)
{
	if (stream == NULL) {
		errno = EBADF;
		return -1;
	}

	//fwrite(stream->write_buffer, 1, stream->write_buffer_index, stream);
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
		for (size_t i = 0; i < open_files_index; i++) {
			fflush(open_files[i]);
		}
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
string_to_flags(const char *restrict mode)
{
	if (mode == NULL) {
		goto bad_mode;
	}
	int flags_val = -1;
	switch (mode[0]) {
	case 'w': {
		if (mode[1] != '\0') {
			if (mode[1] == '+') {
				flags_val = O_RDWR | O_CREATE | O_TRUNC;
				break;
			}
		}
		flags_val = O_WRONLY | O_CREATE | O_TRUNC;
		break;
	}
	case 'r': {
		if (mode[1] != '\0') {
			if (mode[1] == '+') {
				flags_val = O_RDWR;
				break;
			}
		}
		flags_val = O_RDONLY;
		break;
	}
	case 'a': {
		if (mode[1] != '\0') {
			if (mode[1] == '+') {
				flags_val = O_RDWR | O_CREATE | O_APPEND;
				break;
			}
		}
		flags_val = O_WRONLY | O_CREATE | O_APPEND;
		break;
	}
	default:
		goto bad_mode;
	}
	return flags_val;
bad_mode:
	errno = EINVAL;
	return -1;
}

// The "mode" is the "file open mode", not the "permissions mode".
FILE *
fopen(const char *restrict pathname, const char *restrict mode)
{
	const mode_t file_mode = S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH | S_IWUSR | S_IRUSR;
	return fdopen(open(pathname, string_to_flags(mode), file_mode), mode);
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
	FILE *fp = malloc(sizeof(FILE));
	if (fp == NULL)
		return NULL;
	fp->flags = string_to_flags(mode);
	if (fp->flags == -1)
		return NULL;
	fp->mode = S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH | S_IWUSR | S_IRUSR;
	fp->fd = fd;
	fp->eof = false;
	fp->error = false;
	fp->write_buffer = malloc(WRITE_BUFFER_SIZE);
	if (fp->write_buffer == NULL)
		return NULL;
	fp->write_buffer_size = WRITE_BUFFER_SIZE;
	fp->write_buffer_index = 0;
	fp->stdio_flush = true;
	fp->previous_char = EOF;
	fp->buffer_mode = _IOFBF;
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
getc(FILE *stream)
{
	int c;
	if (read(fileno(stream), &c, 1) != 1) {
		return EOF;
	}
	stream->previous_char = c;
	return c;
}

int
getchar(void)
{
	return getc(stdin);
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

	if (stream == NULL) {
		errno = EINVAL;
		return NULL;
	}

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
	if (fp == NULL)
		return;
	fp->write_buffer[fp->write_buffer_index++] = c;
	if (fp &&
		((fp->buffer_mode == _IOFBF &&
		fp->write_buffer_index >= fp->write_buffer_size - 1) ||
	(fp->buffer_mode == _IONBF) ||
	(fp->buffer_mode == _IOLBF && c == '\n'))) {
		flush(fp);
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
	return fputc(c, stdout);
}

static size_t
ansi_noop(const char *s)
{
	return 1;
}

int
vfprintf(FILE *restrict stream, const char *restrict fmt, va_list argp)
{
	int ret = __libc_vprintf_template(fd_putc, ansi_noop, stream, NULL, fmt, argp,
														SIZE_MAX);
	if (stream && stream->stdio_flush)
		flush(stream);
	return ret;
}

int
vsnprintf(char *restrict str, size_t n, const char *restrict fmt, va_list argp)
{
	global_idx = 0;
	int idx = __libc_vprintf_template(string_putc, ansi_noop, NULL, str, fmt, argp,
														 n);
	str[idx] = '\0';
	return idx;
}
int
vsprintf(char *restrict str, const char *restrict fmt, va_list argp)
{
	global_idx = 0;
	return __libc_vprintf_template(string_putc, ansi_noop, NULL, str, fmt, argp,
														SIZE_MAX);
}
int
snprintf(char *restrict str, size_t n, const char *restrict fmt, ...)
{
	int ret;
	va_list listp;
	va_start(listp, fmt);
	ret = vsnprintf(str, n, fmt, listp);
	str[ret] = '\0';
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
	str[ret] = '\0';
	va_end(listp);
	return ret;
}

int
vfscanf(FILE *restrict stream, const char *restrict fmt, va_list argp)
{
	static char static_buffer[__DIRSIZ];
	global_idx_fgetc = 0;
	return __libc_vprintf_template(static_fgetc,
														ansi_noop, stream, static_buffer, fmt, argp, -1);
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
				char *cont = "this has to be initialized to something";
				int d = strtol(str + i, &cont, 10);
				*va_arg(listp, int *) = d;
				i += cont - (str+i) - format_size;
				format_size = 0;
				state = 0;
				break;
			}
			default: {
				if (count > 0)
					count--;
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

__attribute__((format(printf, 2, 3))) int
dprintf(int fd, const char *restrict fmt, ...)
{
	FILE *fp = fdopen(fd, "w");
	if (fp == NULL)
		return -1;
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

__attribute__((format(printf, 1, 2))) int
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
	if (buf != NULL) {
		fflush(stream);
		if (stream != NULL) {
			stream->write_buffer = buf;
			stream->write_buffer_size = n;
		}
	}
	if (!(modes == _IOFBF || modes == _IOLBF || modes == _IONBF)) {
		errno = EINVAL;
		return -1;
	}
	if (stream != NULL) {
		stream->buffer_mode = modes;
	}
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

char *
strerror(int err_no)
{
	return (char *)errno_codes[err_no];
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
	if (stream) {
		return lseek(stream->fd, 0L, SEEK_CUR);
	} else {
		errno = -EBADF;
		return -1;
	}
}

void
rewind(FILE *stream)
{
	if (stream == NULL)
		return;
	(void)fseek(stream, 0L, SEEK_SET);
	clearerr(stream);
}
