#include "stat.h"
#include <errno.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <kernel/include/x86.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>

int errno;
extern char **environ;

char *
strcpy(char *s, const char *t)
{
	char *os;

	os = s;
	while ((*s++ = *t++) != 0)
		;
	return os;
}

char *
strcat(char *dst, const char *src)
{
	int start = strlen(dst);
	int j = 0;
	for (int i = start; i < start + strlen(src) + 1; i++, j++) {
		dst[i] = src[j];
	}
	return dst;
}

int
strcmp(const char *p, const char *q)
{
	while (*p && *p == *q)
		p++, q++;
	return (uint8_t)*p - (uint8_t)*q;
}

int
strncmp(const char *p, const char *q, uint32_t n)
{
	while (n > 0 && *p && *p == *q)
		n--, p++, q++;
	if (n == 0)
		return 0;
	return (uint8_t)*p - (uint8_t)*q;
}

uint32_t
strlen(const char *s)
{
	int n;

	for (n = 0; s[n]; n++)
		;
	return n;
}

void *
memset(void *dst, int c, uint32_t n)
{
	stosb(dst, c, n);
	return dst;
}

char *
strchr(const char *s, char c)
{
	for (; *s; s++)
		if (*s == c)
			return (char *)s;
	return 0;
}
char *
strrchr(const char *s, char c)
{
	for (int i = strlen(s) - 1; i >= 0; i--)
		if (s[i] == c)
			return (char *)s + i;
	return 0;
}

// fill in st from pathname n
__attribute__((nonnull(2))) int
stat(const char *n, struct stat *st)
{
	int fd;
	int r;

	fd = open(n, O_RDONLY);
	if (fd < 0)
		return -1;
	r = fstat(fd, st);
	close(fd);
	return r;
}

DIR *
fopendir(int fd)
{
	if (fd == -1)
		return NULL;
	DIR *dirp = (DIR *)malloc(sizeof(DIR));
	if (dirp == NULL) {
		close(fd);
		return NULL;
	}
	dirp->fd = fd;
	dirp->buffer = NULL;
	dirp->buffer_size = 0;
	dirp->nextptr = NULL;
	return dirp;
}

DIR *
opendir(const char *path)
{
	int fd = open(path, O_RDONLY /*| O_DIRECTORY*/);
	if (fd == -1)
		return NULL;
	return fopendir(fd);
}
int
closedir(DIR *dir)
{
	if (dir == NULL || dir->fd == -1) {
		return -1; // EBADF
	}
	if (dir->buffer != NULL)
		free(dir->buffer);
	int rc = close(dir->fd);
	if (rc == 0)
		dir->fd = -1;
	free(dir);
	return rc;
}

// no way to check for error...sigh....
// atoi("0") == 0
// atoi("V") == 0
// "V" != "0"
int
atoi(const char *s)
{
	int n = 0;

	while ('0' <= *s && *s <= '9')
		n = n * 10 + *s++ - '0';
	return n;
}
int
atoi_base(const char *s, uint32_t base)
{
	int n = 0;

	if (base <= 10) {
		while ('0' <= *s && *s <= '9')
			n = n * base + *s++ - '0';
		return n;
	} else if (base <= 16) {
		while (('0' <= *s && *s <= '9') || ('a' <= *s && *s <= 'f')) {
			if ('0' <= *s && *s <= '9')
				n = n * base + *s++ - '0';
			if ('a' <= *s && *s <= 'f')
				n = n * base + *s++ - 'a';
		}
		return n;
	} else {
		return 0;
	}
}

void *
memmove(void *vdst, const void *vsrc, uint32_t n)
{
	char *dst;
	const char *src;

	dst = vdst;
	src = vsrc;
	while (n-- > 0)
		*dst++ = *src++;
	return vdst;
}

void *
memcpy(void *dst, const void *src, uint32_t n)
{
	return memmove(dst, src, n);
}

char *
strncpy(char *dst, const char *src, int n)
{
	char *my_dst;
	const char *my_src;

	my_dst = dst;
	my_src = src;
	while (n-- > 0)
		*my_dst++ = *my_src++;
	*my_dst++ = '\0';
	return dst;
}

void
assert_fail(const char *assertion, const char *file, int lineno,
						const char *func)
{
	fprintf(stderr, "%s:%d: %s: Assertion `%s' failed.\n", file, lineno, func,
					assertion);
	fprintf(stderr, "Aborting.\n");
	exit(-1);
}
int
isdigit(int c)
{
	return '0' <= c && c <= '9';
}

int
isspace(int c)
{
	return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' ||
				 c == '\v';
}
static char *restrict __strtok_token = NULL;
/*
 * Break a string into a sequence of zero or more nonempty tokens.
 * If the same string is being parsed as the previous invocation, str must be NULL.
 */
char *
strtok(char *restrict str, const char *restrict delimeter)
{
	if (str == NULL && __strtok_token)
		return NULL;

	if (str != NULL) {
		__strtok_token = str;
	}

	if (__strtok_token == NULL) {
		return NULL;
	}

	while (*__strtok_token && strchr(delimeter, *__strtok_token) != NULL) {
		__strtok_token++;
	}

	if (*__strtok_token == '\0') {
		return NULL;
	}

	char *start = __strtok_token;
	while (*__strtok_token && strchr(delimeter, *__strtok_token) == NULL) {
		__strtok_token++;
	}

	// The input string gets consumed from the input, so we modify it here.
	if (*__strtok_token) {
		*__strtok_token++ = '\0';
	}

	return start;
}

// Fun fact: setenv sets errno, but getenv does not :^)
char *
getenv(const char *name)
{
	for (size_t i = 0; environ[i] != NULL; i++) {
		char *equals = strchr(environ[i], '=');
		if (equals == NULL)
			continue; // Resilient. (Is this in the spec?)
		size_t this_env_length = equals - environ[i];
		// If it's not the same length, we don't even bother comparing.
		if (strlen(name) != this_env_length)
			continue;
		if (strncmp(name, environ[i], this_env_length) == 0) {
			return equals + 1;
		}
	}
	return NULL;
}

// Duplicate a string
// Caller frees the string.
char *
strdup(const char *s)
{
	if (s == NULL)
		return NULL;
	char *new_s = malloc(strlen(s) + 1);
	if (new_s == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	strncpy(new_s, s, strlen(s));
	return new_s;
}

typedef void (*atexit_handler)(void);
static atexit_handler atexit_handlers[ATEXIT_MAX] = { NULL };

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
