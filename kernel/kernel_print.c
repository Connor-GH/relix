#include "spinlock.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
printint(void (*put_function)(char, char *), char *put_func_buf, int64_t xx,
         int base, bool sgn, int flags, int padding)
{
	static const char digits[] = "0123456789abcdef";
	char buf[4096];
	int i = 0;
	int neg = 0;
	uint64_t x;
	int ret;

	if (sgn && xx < 0) {
		neg = 1;
		x = -xx;
	} else {
		x = xx;
	}
	int numlen = 1;
	uint64_t x_copy = x;
	while ((x_copy /= base) != 0) {
		numlen++;
	}

	x_copy = x;

	if (IS_SET(flags, FLAG_ALTFORM)) {
		if (base == 16 || base == 2) {
			padding -= 2;
		} else if (base == 8) {
			if (x_copy != 0 && padding < numlen) {
				padding++;
			}
		}
	}
	if (IS_SET(flags, FLAG_LJUST) && !IS_SET(flags, FLAG_PRECISION)) {
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
	if (IS_SET(flags, FLAG_ALTFORM)) {
		if (x_copy != 0 || (x_copy == 0 && (IS_SET(flags, FLAG_PRECISION) ||
		                                    IS_SET(flags, FLAG_PADZERO)))) {
			if (base == 16) {
				buf[i++] = 'x';
			} else if (base == 2) {
				buf[i++] = 'b';
			}
			buf[i++] = '0';
		}
	}
	// append negative/positive sign
	if (neg) {
		buf[i++] = '-';
	} else if (IS_SET(flags, FLAG_SIGN)) {
		buf[i++] = '+';
	}
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
	ret = i;

	while (--i >= 0) {
		put_function(buf[i], put_func_buf);
	}
	return ret;
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

static void
print_string(void (*put_function)(char c, char *buf), char *s, int flags,
             char *restrict buf, int str_pad)
{
	if (s == NULL) {
		s = "(null)";
	}
	size_t len;
	if (strcmp(s, "") == 0) {
		len = 0;
	} else {
		len = strlen(s);
	}
	while (len != 0 && *s != 0) {
		put_function(*s, buf);
		s++;
	}
	if (IS_SET(flags, FLAG_LJUST) && len < str_pad) {
		for (int i = 0; i < str_pad - len; i++) {
			put_function(' ', buf);
		}
	}
}

// Print to the given fd. Only understands %d, %x, %p, %s.
int
kernel_vprintf_template(void (*put_function)(char c, char *buf),
                        size_t (*ansi_func)(const char *), char *restrict buf,
                        const char *fmt, va_list argp, struct spinlock *lock,
                        bool locking, size_t print_n_chars)
{
	char *s;
	int c = 0, i = 0, state = 0;
	int flags = 0;
	int str_pad = 0;

	if (locking) {
		acquire(lock);
	}
	for (; fmt[i]; i++) {
		if (i >= print_n_chars) {
			break;
		}
		// 'floor' character down to bottom 255 chars
		c = fmt[i] & 0xff;
		if (state == 0) {
			if (c == '%') {
				state = '%';
			} else if (c == '\033') {
				if (ansi_func != NULL) {
					i += ansi_func(fmt + i);
				} else {
					put_function(c, buf);
				}
				continue;
			} else {
				put_function(c, buf);
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
				if (IS_SET(flags, FLAG_PADZERO)) {
					flags ^= FLAG_PADZERO;
				}
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
			case 'z':
			case 'l':
				flags |= FLAG_LONG;
				goto skip_state_reset;
			case 'i':
			case 'd': {
				if (IS_SET(flags, FLAG_LONG)) {
					long ld = va_arg(argp, long);
					printint(put_function, buf, ld, 10, true, flags, str_pad);
				} else {
					int d = va_arg(argp, int);
					printint(put_function, buf, d, 10, true, flags, str_pad);
				}
				break;
			}
			case 'b': {
				if (IS_SET(flags, FLAG_LONG)) {
					unsigned long lb = va_arg(argp, unsigned long);
					printint(put_function, buf, lb, 2, false, flags, str_pad);
				} else {
					unsigned int b = va_arg(argp, unsigned int);
					printint(put_function, buf, b, 2, false, flags, str_pad);
				}
				break;
			}
			case 'u': {
				if (IS_SET(flags, FLAG_LONG)) {
					unsigned long lu = va_arg(argp, unsigned long);
					printint(put_function, buf, lu, 10, false, flags, str_pad);
				} else {
					unsigned int u = va_arg(argp, unsigned int);
					printint(put_function, buf, u, 10, false, flags, str_pad);
				}
				break;
			}
			case 'x': {
				if (IS_SET(flags, FLAG_LONG)) {
					unsigned long lx = va_arg(argp, unsigned long);
					printint(put_function, buf, lx, 16, false, flags, str_pad);
				} else {
					unsigned int x = va_arg(argp, unsigned int);
					printint(put_function, buf, x, 16, false, flags, str_pad);
				}
				break;
			}
			case 'p': {
				flags |= FLAG_ALTFORM;
				uintptr_t x = (uintptr_t)va_arg(argp, void *);
				printint(put_function, buf, x, 16, false, flags, str_pad);
				break;
			}
			case 'o': {
				int x = va_arg(argp, int);
				printint(put_function, buf, x, 8, false, flags, str_pad);
				break;
			}
			case 's': {
				s = va_arg(argp, char *);
				print_string(put_function, s, flags, buf, str_pad);
				break;
			}
			case 'c': {
				int c_ = va_arg(argp, int);
				put_function(c_, buf);
				break;
			}
			case '%':
				put_function(c, buf);
				break;
			// Unknown % sequence.  Print it to draw attention.
			default:
				put_function('%', buf);
				put_function(c, buf);
				break;
			}
			str_pad = 0;
			state = 0;
			flags = 0;
skip_state_reset:; // state = '%' if set
		}
	}
	if (locking) {
		release(lock);
	}
	return i;
}
