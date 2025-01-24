#include "kernel/include/param.h"
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
				mode_val = O_RDWR | O_CREATE /*| O_APPEND*/;
				break;
			}
		}
		mode_val = O_WRONLY | O_CREATE /*| O_APPEND*/;
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

enum {
	FLAG_PADZERO = 1 << 0,
	FLAG_ALTFORM = 1 << 1,
	FLAG_LJUST = 1 << 2,
	FLAG_BLANK = 1 << 3,
	FLAG_SIGN = 1 << 4,
	FLAG_LONG = 1 << 5,
};
#define IS_SET(x, flag) (bool)((x & flag) == flag)

static void
printint(void (*put_function)(FILE *, char, char *), char *put_func_buf,
				 FILE *fp, int64_t xx, int base, bool sgn, int flags, int padding)
{
	static const char digits[] = "0123456789ABCDEF";
	char buf[64];
	int i = 0;
	int neg = 0;
	uint64_t x;

	if (sgn && xx < 0) {
		neg = 1;
		x = -xx;
	} else {
		x = xx;
	}
	int numlen = 1;
	int x_copy = x;
	while ((x_copy /= base) != 0)
		numlen++;

	if (IS_SET(flags, FLAG_LJUST)) {
		if (base == 16 && IS_SET(flags, FLAG_ALTFORM))
			padding -= 2;
		while (i < padding - numlen) {
			buf[i++] = ' ';
		}
	}
	do {
		buf[i++] = digits[x % base];
	} while ((x /= base) != 0);
	// pad for zeroes (and blanks)
	if (IS_SET(flags, FLAG_PADZERO)) {
		while (i < padding) {
			buf[i++] = '0';
		}
	}
	if (base == 16 && IS_SET(flags, FLAG_ALTFORM)) {
		buf[i++] = 'x';
		buf[i++] = '0';
	}
	// append negative/positive sign
	if (neg)
		buf[i++] = '-';
	else if (IS_SET(flags, FLAG_SIGN))
		buf[i++] = '+';
	if (IS_SET(flags, FLAG_BLANK))
		buf[i++] = ' ';

	if (!IS_SET(flags, FLAG_LJUST) && !IS_SET(flags, FLAG_PADZERO) &&
			padding != 0) {
		while (i < padding) {
			buf[i++] = ' ';
		}
	}

	while (--i >= 0)
		put_function(fp, buf[i], put_func_buf);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
static void
vprintf_internal(void (*put_function)(FILE *fp, char c, char *buf), FILE *fp,
								 char *restrict buf, const char *fmt, va_list *argp)
{
	global_idx = 0;
	char *s;
	int c, i, state;
	int flags = 0;
	int str_pad = 0;

	state = 0;
	for (i = 0; fmt[i]; i++) {
		// 'floor' character down to bottom 255 chars
		c = fmt[i] & 0xff;
		if (state == 0) {
			if (c == '%') {
				state = '%';
			} else {
				put_function(fp, c, buf);
			}
		} else if (state == '%') {
			switch (c) {
			case '0': {
				if (str_pad == 0) {
					flags |= FLAG_PADZERO;
					goto skip_state_reset;
				} else {
					goto numerical_padding;
				}
			}
			case '1' ... '9':
numerical_padding:
				str_pad = str_pad * 10 + (c - '0');
				// soon...
				//if (IS_SET(flags, FLAG_PADZERO)) {}
				// str_pad = c - '0';
				goto skip_state_reset;
				break;
			case '#':
				flags |= FLAG_ALTFORM;
				goto skip_state_reset;
				break;
			case '-':
				flags |= FLAG_LJUST;
				if (IS_SET(flags, FLAG_PADZERO))
					flags ^= FLAG_PADZERO;
				if (fmt[i + 1] && (fmt[i + 1] - '0' >= 0)) {
					str_pad = fmt[++i] - '0';
				}
				goto skip_state_reset;
				break;
			case ' ':
				flags |= FLAG_BLANK;
				goto skip_state_reset;
				break;
			case '+':
				flags |= FLAG_SIGN;
				goto skip_state_reset;
			case 'l':
				flags |= FLAG_LONG;
				goto skip_state_reset;
			case 'i':
			case 'd': {
				if (IS_SET(flags, FLAG_LONG)) {
					long ld = va_arg(*argp, long);
					printint(put_function, buf, fp, ld, 10, true, flags, str_pad);
					str_pad = 0;
				} else {
					int d = va_arg(*argp, int);
					printint(put_function, buf, fp, d, 10, true, flags, str_pad);
					str_pad = 0;
				}
				break;
			}
			case 'u': {
				if (IS_SET(flags, FLAG_LONG)) {
					long lu = va_arg(*argp, unsigned long);
					printint(put_function, buf, fp, lu, 10, false, flags, str_pad);
					str_pad = 0;
				} else {
					unsigned int u = va_arg(*argp, unsigned int);
					printint(put_function, buf, fp, u, 10, false, flags, str_pad);
					str_pad = 0;
				}
				break;
			}
			case 'x': {
				if (IS_SET(flags, FLAG_LONG)) {
					unsigned long lx = va_arg(*argp, unsigned long);
					printint(put_function, buf, fp, lx, 16, true, flags, str_pad);
					str_pad = 0;
				} else {
					unsigned int x = va_arg(*argp, unsigned int);
					printint(put_function, buf, fp, x, 16, true, flags, str_pad);
					str_pad = 0;
				}
				break;
			}
			case 'p': {
				uintptr_t x = (uintptr_t)va_arg(*argp, void *);
				printint(put_function, buf, fp, x, 16, false, flags, str_pad);
				str_pad = 0;
				break;
			}
			case 'o': {
				int x = va_arg(*argp, int);
				printint(put_function, buf, fp, x, 8, false, flags, str_pad);
				str_pad = 0;
				break;
			}
			case 's': {
				s = va_arg(*argp, char *);
				if (s == 0)
					s = "(null)";
				size_t len = strlen(s);
				while (*s != 0) {
					put_function(fp, *s, buf);
					s++;
				}
				if (IS_SET(flags, FLAG_LJUST) && len < str_pad) {
					for (int _ = 0; _ < str_pad - len; _++)
						put_function(fp, ' ', buf);
				}
				str_pad = 0;
				break;
			}
			case 'c': {
				int c_ = va_arg(*argp, int);
				put_function(fp, c_, buf);
				break;
			}
			case '%':
				put_function(fp, c, buf);
				break;
			// Unknown % sequence.  Print it to draw attention.
			default:
				put_function(fp, '%', buf);
				put_function(fp, c, buf);
				break;
			}
			state = 0;
skip_state_reset:; // state = '%' if set
		}
	}
}

void
vfprintf(FILE *restrict stream, const char *fmt, va_list *argp)
{
	vprintf_internal(fd_putc, stream, NULL, fmt, argp);
	flush(stream);
}

void
vsprintf(char *restrict str, const char *restrict fmt, va_list *argp)
{
	vprintf_internal(string_putc, 0, str, fmt, argp);
}
void
sprintf(char *restrict str, const char *restrict fmt, ...)
{
	va_list listp;
	va_start(listp, fmt);
	vprintf_internal(string_putc, 0, str, fmt, &listp);
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
	printf("%s: %s\n", s, errno_codes[errno]);
}

const char *
strerror(int err_no)
{
	return errno_codes[err_no];
}
