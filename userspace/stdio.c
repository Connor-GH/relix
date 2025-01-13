#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

static uint32_t global_idx = 0;

#define WRITE_BUFFER_SIZE 256
static char *write_buffer = NULL;
static size_t write_buffer_position = 0;
void
__init_stdio(void)
{
	write_buffer = malloc(WRITE_BUFFER_SIZE);
}
static void
flush(int fd)
{
	writev(fd, &(const struct iovec){write_buffer, write_buffer_position}, 1);
	write_buffer_position = 0;
}

int
fflush(FILE *stream)
{
	if (stream == NULL) {
		fflush(stdout);
		fflush(stderr);
	}
	int fd = fileno(stream);
	if (fd < 0) {
		return EOF;
	}
	flush(fd);
	return 0;
}

int
fileno(FILE *stream)
{
	// TODO make actual FILE * structure
	if (stream != NULL)
		return *stream;
	errno = EBADF;
	return -1;
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
fd_putc(int fd, char c, char *buf)
{
	if (write_buffer_position >= WRITE_BUFFER_SIZE-1) {
		flush(fd);
	} else {
		write_buffer[write_buffer_position++] = c;
	}
}
static void
string_putc(int fd, char c, char *buf)
{
	buf[global_idx] = c;
	global_idx++;
}
int
fputc(int c, FILE *stream)
{
	fd_putc(fileno(stream), c, NULL);
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
printint(void (*put_function)(int, char, char *), char *put_func_buf, int fd,
				 int64_t xx, int base, bool sgn, int flags, int padding)
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
		put_function(fd, buf[i], put_func_buf);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
static void
vprintf_internal(void (*put_function)(int fd, char c, char *buf), int fd,
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
				put_function(fd, c, buf);
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
					printint(put_function, buf, fd, ld, 10, true, flags, str_pad);
					str_pad = 0;
				} else {
					int d = va_arg(*argp, int);
					printint(put_function, buf, fd, d, 10, true, flags, str_pad);
					str_pad = 0;
				}
				break;
			}
			case 'u': {
				if (IS_SET(flags, FLAG_LONG)) {
					long lu = va_arg(*argp, unsigned long);
					printint(put_function, buf, fd, lu, 10, false, flags, str_pad);
					str_pad = 0;
				} else {
					unsigned int u = va_arg(*argp, unsigned int);
					printint(put_function, buf, fd, u, 10, false, flags, str_pad);
					str_pad = 0;
				}
				break;
			}
			case 'x':
			case 'p': {
				int x = va_arg(*argp, int);
				printint(put_function, buf, fd, x, 16, false, flags, str_pad);
				str_pad = 0;
				break;
			}
			case 'o': {
				int x = va_arg(*argp, int);
				printint(put_function, buf, fd, x, 8, false, flags, str_pad);
				str_pad = 0;
				break;
			}
			case 's': {
				s = va_arg(*argp, char *);
				if (s == 0)
					s = "(null)";
				size_t len = strlen(s);
				while (*s != 0) {
					put_function(fd, *s, buf);
					s++;
				}
				if (IS_SET(flags, FLAG_LJUST) && len < str_pad) {
					for (int _ = 0; _ < str_pad - len; _++)
						put_function(fd, ' ', buf);
				}
				str_pad = 0;
				break;
			}
			case 'c': {
				int c_ = va_arg(*argp, int);
				put_function(fd, c_, buf);
				break;
			}
			case '%':
				put_function(fd, c, buf);
				break;
			// Unknown % sequence.  Print it to draw attention.
			default:
				put_function(fd, '%', buf);
				put_function(fd, c, buf);
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
	vprintf_internal(fd_putc, fileno(stream), NULL, fmt, argp);
	flush(fileno(stream));
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
