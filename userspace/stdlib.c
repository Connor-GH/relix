#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// no way to check for error...sigh....
// atoi("0") == 0
// atoi("V") == 0
// "V" != "0"
int
atoi(const char *s)
{
	int n = 0;

	while ('0' <= *s && *s <= '9') {
		n = n * 10 + *s++ - '0';
	}
	return n;
}

long long
strtoll(const char *restrict s, char **restrict endptr, int base)
{
	long long num = 0;
	bool positive = true;
	const char base_uppercase[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const char base_lowercase[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	if ((base != 0 && base < 2) || base > 36) {
		errno = EINVAL;
		return 0;
	}
	size_t i = 0;
	// skip whitespace
	while (isspace(s[i])) {
		i++;
	}
	if (s[i] == '+') {
		i++;
		positive = true;
	} else if (s[i] == '-') {
		i++;
		positive = false;
	}
	if (base == 0 || base == 16) {
		// Hexadecimal.
		if (strncmp(s + i, "0x", 2) == 0 || strncmp(s + i, "0X", 2) == 0) {
			base = 16;
			i += 2;
			// Octal.
		} else if (strncmp(s + i, "0", 1) == 0) {
			i++;
			base = 8;
		}
	}
	if (base <= 10) {
		while ('0' <= s[i] && s[i] <= '9') {
			num = num * base + s[i++] - '0';
		}
		if (endptr != NULL && *endptr != NULL) {
			*endptr = (char *)(s + i + 1);
		}
		return num;
	} else if (base <= 36) {
		while (s[i] != '\0' &&
		       (('0' <= s[i] && s[i] <= '9') || ('a' <= s[i] && s[i] <= 'z') ||
		        ('A' <= s[i] && s[i] <= 'Z'))) {
			if ('0' <= s[i] && s[i] <= '9') {
				num = num * base + s[i++] - '0';
			}
			if ('a' <= s[i] && s[i] <= 'z') {
				num = num * base + s[i++] - 'a';
			} else if ('A' <= s[i] && s[i] <= 'Z') {
				num = num * base + s[i++] - 'A';
			}
		}
		if (endptr != NULL && *endptr != NULL) {
			*endptr = (char *)(s + i + 1);
		}
		return num;
	}
	errno = EINVAL;
	return 0;
}

long
strtol(const char *restrict s, char **restrict nptr, int base)
{
	return (long)strtoll(s, nptr, base);
}

long
atol(const char *s)
{
	return strtol(s, NULL, 0);
}

long long
atoll(const char *s)
{
	return strtoll(s, NULL, 0);
}

int
atoi_base(const char *s, int base)
{
	return (int)strtoll(s, NULL, base);
}

unsigned long long
strtoull(const char *s, char **nptr, int base)
{
	return strtoll(s, nptr, base);
}

unsigned long
strtoul(const char *s, char **endptr, int base)
{
	return strtoull(s, endptr, base);
}

// Implmentation taken from the C Programming Language
double
atof(const char *nptr)
{
	double val, power;
	int i, sign;
	// Skip whitespace.
	for (i = 0; isspace(nptr[i]); i++)
		;

	sign = (nptr[i] == '-') ? -1 : 1;

	// Look at sign and determine whether it is positive or negative.
	if (nptr[i] == '+' || nptr[i] == '-') {
		i++;
	}

	// Next are the digits.
	for (val = 0.0; isdigit(nptr[i]); i++) {
		val = 10.0 * val + (nptr[i] - '0');
	}

	// Decimal point.
	if (nptr[i] == '.') {
		i++;
	}
	// Build up the fraction part the same as the
	// integer part, but divide by 'power' to account
	// for it. e.g. 1.23 is internally represented as
	// val = 123 and power = 100.
	for (power = 1.0; isdigit(nptr[i]); i++) {
		val = 10.0 * val + (nptr[i] - '0');
		power *= 10.0;
	}
	return sign * val / power;
}

static uint64_t s_next_rand = 1;

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/rand.html
// POSIX implementation.
int
rand(void)
{
	s_next_rand = s_next_rand * 1103515245 + 12345;
	return ((unsigned int)(s_next_rand / ((RAND_MAX + 1) * 2)) % (RAND_MAX + 1));
}

// https://pubs.opengroup.org/onlinepubs/9699919799/functions/srand.html
void
srand(unsigned int seed)
{
	s_next_rand = seed;
}

int
system(const char *command)
{
	fprintf(stderr, "FIXME: system(\"%s\")\n", command);
	return 1;
}

int
raise(int sig)
{
	return kill(getpid(), sig);
}

__attribute__((noreturn)) void
abort(void)
{
	raise(SIGABRT);
	_exit(1);
}

typedef void (*atexit_handler)(void);
static atexit_handler atexit_handlers[ATEXIT_MAX] = { NULL };

__attribute__((noreturn)) void
exit(int status)
{
	for (int i = ATEXIT_MAX - 1; i >= 0; i--) {
		if (atexit_handlers[i] != NULL) {
			atexit_handlers[i]();
		}
	}
	_exit(status);
}

int
atexit(void (*function)(void))
{
	for (int i = 0; i < ATEXIT_MAX; i++) {
		if (atexit_handlers[i] == NULL) {
			atexit_handlers[i] = function;
			return 0;
		}
	}
	// IEEE Std 1003.1-2024 (POSIX.1-2024):
	// "Upon successful completion, atexit() shall return 0;"
	// "otherwise, it shall return a non-zero value."
	return -1;
}
