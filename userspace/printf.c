#include <stdarg.h>
#include "printf.h"
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

enum {
	FLAG_PADZERO = 1 << 0,
	FLAG_ALTFORM = 1 << 1,
	FLAG_LJUST = 1 << 2,
	FLAG_BLANK = 1 << 3,
	FLAG_SIGN = 1 << 4,
	FLAG_LONG = 1 << 5,
	FLAG_PRECISION = 1 << 6,
};
#define IS_SET(x, flag) (bool)((x & flag) == flag)

static int
print_string(void (*put_function)(FILE *fp, char c, char *buf), char *s,
						 int flags, FILE *fp, char *restrict buf, int str_pad, size_t print_n_chars);
static int
printint(void (*put_function)(FILE *, char, char *), char *put_func_buf,
				 FILE *fp, int64_t xx, int base, bool sgn, int flags, int padding)
{
	static const char digits[] = "0123456789abcdef";
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

	if (IS_SET(flags, FLAG_LJUST) && !IS_SET(flags, FLAG_PRECISION)) {
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
	if (IS_SET(flags, FLAG_BLANK)) {
		while (i < padding) {
			buf[i++] = ' ';
		}
	} else if (IS_SET(flags, FLAG_PRECISION)) {
		while (i < padding) {
			buf[i++] = '0';
		}
	}

	if (!IS_SET(flags, FLAG_LJUST) && !IS_SET(flags, FLAG_PADZERO) &&
			padding != 0) {
		while (i < padding) {
			buf[i++] = ' ';
		}
	}

	while (--i >= 0)
		put_function(fp, buf[i], put_func_buf);
	return padding > numlen ? padding : numlen;
}

static uint64_t
pow_10(int n)
{
	uint64_t result = 1;
	for (int i = 0; i < n; i++) {
		result *= 10;
	}
	return result;
}

static int
print_string(void (*put_function)(FILE *fp, char c, char *buf), char *s,
						 int flags, FILE *fp, char *restrict buf, int str_pad, size_t print_n_chars)
{
	if (s == 0)
		s = "(null)";
	size_t len;
	int written = 0;
	if (strcmp(s, "") == 0) {
			len = 0;
	} else
			len = strnlen(s, print_n_chars);
	while (len != 0 && *s != 0) {
		put_function(fp, *s, buf);
		written++;
		s++;
	}
	if (IS_SET(flags, FLAG_LJUST) && len < str_pad) {
		for (int _ = 0; _ < str_pad - len; _++) {
			put_function(fp, ' ', buf);
			written++;
		}
	}
	return written;
}

static int
print_double(void (*put_function)(FILE *fp, char c, char *buf), double num,
						 int flags, FILE *fp, char *restrict buf, int str_pad, int base)
{
	int written = 0;
	/* Print the num before the decimal point. */
	written = printint(put_function, buf, fp, (uint64_t)num, base, true, flags, 0);
	put_function(fp, '.', buf);
	written++;

	double fraction = (num - (uint64_t)num);
	fraction *= pow_10(str_pad);
	uint64_t fraction_as_integer = (uint64_t)(fraction + 0.5);
	written += printf("%0*lu", str_pad, fraction_as_integer);
	return written;
}


// Print to the given fd. Only understands %d, %x, %p, %s.
int
__libc_vprintf_template(void (*put_function)(FILE *fp, char c, char *buf),
								 size_t (*ansi_func)(const char *), FILE *fp,
								 char *restrict buf, const char *fmt, va_list argp,
								 size_t print_n_chars)
{
	char *s;
	int c = 0, i = 0, state = 0;
	int flags = 0;
	int str_pad = 0;
	int count = 0;

	for (; fmt[i]; i++) {
		if (count >= print_n_chars)
			break;
		// 'floor' character down to bottom 255 chars
		c = fmt[i] & 0xff;
		if (state == 0) {
			if (c == '%') {
				state = '%';
			} else if (c == '\033') {
				i += ansi_func(fmt + i);
				continue;
			} else {
				put_function(fp, c, buf);
				count++;
			}
		} else if (state == '%') {
			switch (c) {
			case '.': {
				if (str_pad == 0) {
					flags |= FLAG_PRECISION;
					goto skip_state_reset;
				} else {
					goto numerical_padding;
				}
			}
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
			case '*':
				str_pad = va_arg(argp, int);
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
					long ld = va_arg(argp, long);
					count += printint(put_function, buf, fp, ld, 10, true, flags, str_pad);

				} else {
					int d = va_arg(argp, int);
					count += printint(put_function, buf, fp, d, 10, true, flags, str_pad);
				}
				break;
			}
			case 'b': {
				if (IS_SET(flags, FLAG_LONG)) {
					long lb = va_arg(argp, unsigned long);
					count += printint(put_function, buf, fp, lb, 2, false, flags, str_pad);
				} else {
					unsigned int b = va_arg(argp, unsigned int);
					count += printint(put_function, buf, fp, b, 2, false, flags, str_pad);
				}
				break;
			}
			case 'g':
			case 'f': {
				double d = va_arg(argp, double);
				count += print_double(put_function, d, flags, fp, buf,
								 str_pad > 0 ? str_pad : 6,
								 IS_SET(flags, FLAG_ALTFORM) ? 16 : 10);
				break;
			}
			case 'u': {
				if (IS_SET(flags, FLAG_LONG)) {
					long lu = va_arg(argp, unsigned long);
					count += printint(put_function, buf, fp, lu, 10, false, flags, str_pad);
				} else {
					unsigned int u = va_arg(argp, unsigned int);
					count += printint(put_function, buf, fp, u, 10, false, flags, str_pad);
				}
				break;
			}
			case 'x': {
				if (IS_SET(flags, FLAG_LONG)) {
					unsigned long lx = va_arg(argp, unsigned long);
					count += printint(put_function, buf, fp, lx, 16, false, flags, str_pad);
				} else {
					unsigned int x = va_arg(argp, unsigned int);
					count += printint(put_function, buf, fp, x, 16, false, flags, str_pad);
				}
				break;
			}
			case 'p': {
				flags |= FLAG_ALTFORM;
				uintptr_t x = (uintptr_t)va_arg(argp, void *);
				count += printint(put_function, buf, fp, x, 16, false, flags, str_pad);
				break;
			}
			case 'o': {
				int x = va_arg(argp, int);
				count += printint(put_function, buf, fp, x, 8, false, flags, str_pad);
				break;
			}
			case 's': {
				s = va_arg(argp, char *);
				count += print_string(put_function, s, flags, fp, buf, str_pad, print_n_chars);
				break;
			}
			case 'c': {
				int c_ = va_arg(argp, int);
				put_function(fp, c_, buf);
				count++;
				break;
			}
			case '%':
				put_function(fp, c, buf);
				count++;
				break;
			// Unknown % sequence.  Print it to draw attention.
			default:
				put_function(fp, '%', buf);
				put_function(fp, c, buf);
				count++;
				break;
			}
			str_pad = 0;
			state = 0;
			flags = 0;
skip_state_reset:; // state = '%' if set
		}
	}
	return count;
}
