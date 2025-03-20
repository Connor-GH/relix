#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/include/x86.h"
#include <stddef.h>
#include <ctype.h>

#define min(x, y) ((x) < (y) ? (x) : (y))

char *
strcpy(char *s, const char *t)
{
	char *os;

	os = s;
	while ((*s++ = *t++) != 0)
		;
	return os;
}

int
strcmp(const char *p, const char *q)
{
	size_t np = strlen(p);
	size_t nq = strlen(q);
	return strncmp(p, q, (np > nq ? nq : np) + 1);
}

char *
strstr(const char *s1, const char *s2)
{
	const char *p1 = s1;
	const char *p2;

	while (*s1) {
		p2 = s2;
		while (*p2 != '\0' && (*p1 == *p2)) {
			p1++;
			p2++;
		}
		if (*p2 == '\0') {
			return (char *)s1;
		}
		s1++;
		p1 = s1;
	}
	return NULL;
}
int
strncmp(const char *p, const char *q, size_t n)
{
	while (n > 0 && *p && *p == *q)
		n--, p++, q++;
	if (n == 0)
		return 0;
	return (uint8_t)*p - (uint8_t)*q;
}

int
strncasecmp(const char *p, const char *q, size_t n)
{
	while (n > 0 && *p && tolower(*p) == tolower(*q))
		n--, p++, q++;
	if (n == 0)
		return 0;
	return (uint8_t)*p - (uint8_t)*q;
}
int
strcasecmp(const char *p, const char *q)
{
	return strncasecmp(p, q, min(strlen(p), strlen(q)));
}

size_t
strlen(const char *s)
{
	int n;

	for (n = 0; s && s[n]; n++)
		;
	return n;
}
size_t
strnlen(const char *s, size_t size)
{
	int n;

	for (n = 0; n < size && s && s[n]; n++)
		;
	return n;
}

char *
strchr(const char *s, int c)
{
	for (; *s; s++)
		if (*s == c)
			return (char *)s;
	return 0;
}
char *
strrchr(const char *s, int c)
{
	for (int i = strlen(s) - 1; i >= 0; i--)
		if (s[i] == c)
			return (char *)s + i;
	return 0;
}
char *
strncpy(char *dst, const char *src, size_t n)
{
	char *start = dst;
	while (n > 0 && *src != '\0') {
		*dst++ = *src++;
    n--;
  }

	while (n > 0) {
    *dst++ = '\0';
    n--;
  }

  return start;
}
char *
stpncpy(char *dst, const char *src, size_t n)
{
	while (n-- > 0 && (*dst++ = *src++) != 0)
		;
	while (n-- > 1)
		*dst++ = '\0';
	return dst;
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

void *
memset(void *dst, int c, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((char *)dst)[i] = c;
	}
	return dst;
}

int
memcmp(const void *v1, const void *v2, size_t n)
{
	const uint8_t *dst, *s2;

	dst = v1;
	s2 = v2;
	while (n-- > 0) {
		if (*dst != *s2)
			return *dst - *s2;
		dst++, s2++;
	}

	return 0;
}
__deprecated("Removed in POSIX.1-2008") int bcmp(const void *v1, const void *v2,
																								 size_t n)
{
	return memcmp(v1, v2, n);
}

// Based on code from https://libc11.org/string/memmove.html (public domain)
void *
memmove(void *dst, const void *src, size_t n)
{
	char *dest = (char *)dst;
	const char *source = (const char *)src;
	if (dest == source || n == 0)
		return dst;

	if (dest == NULL)
		return NULL;
	// If source is lower than dest in memory,
	// we don't have to worry about clobbering it going forwards.
	if (dest < source) {
		while (n--) {
			*dest++ = *source++;
		}
	} else {
		// We may clobber going forwards, so let's go backwards.
		source += n;
		dest += n;
		while (n--) {
			*--dest = *--source;
		}
	}
	return dst;
}

void *
mempcpy(void *dst, const void *src, size_t n)
{
	return memcpy(dst, src, n) + n;
}

void *
memcpy(void *dst, const void *src, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		((char *)dst)[i] = ((char *)src)[i];
	}
	return dst;
}

char *
stpcpy(char *restrict dst, const char *restrict src)
{
	char *p;

	p = mempcpy(dst, src, strlen(src));
	*p = '\0';

	return p;
}

char *
strcat(char *restrict dst, const char *restrict src)
{
	stpcpy(dst + strlen(dst), src);
	return dst;
}

char *
strncat(char *dst, const char *src, size_t n)
{
	size_t start = strlen(dst);
	size_t j = 0;
	for (size_t i = start; i < start + n; i++, j++) {
		dst[i] = src[j];
	}
	return dst;
}

// Like strncpy but guaranteed to NUL-terminate.
char *
__safestrcpy(char *s, const char *t, size_t n)
{
	char *os;

	os = s;
	if (n <= 0)
		return os;
	while (--n > 0 && (*s++ = *t++) != 0)
		;
	*s = 0;
	return os;
}
char *
__strlcpy_nostrlen(char *dst, const char *src, size_t dst_len, size_t src_len)
{
	// this is guaranteed to NUL-terminate even on strings without strlen.
	// only issue is lack of verification of string length.
	return __safestrcpy(dst, src, min(dst_len, src_len));
}

size_t
strcspn(const char *s1, const char *s2)
{
	size_t len = 0;
	const char *p;

	while (s1[len]) {
		p = s2;

		while (*p) {
			if (s1[len] == *p++) {
				return len;
			}
		}

		len++;
	}

	return len;
}

size_t
strspn(const char *s1, const char *s2)
{
	size_t len = 0;
	const char *p;

	while (s1[len]) {
		p = s2;

		while (*p) {
			if (s1[len] == *p) {
				break;
			}

			++p;
		}

		if (!*p) {
			return len;
		}

		++len;
	}

	return len;
}

char *
strpbrk(const char *s1, const char *s2)
{
	const char *p1 = s1;
	const char *p2;
	while (*p1 != '\0') {
		p2 = s2;
		while (*p2 != '\0') {
			if (*p1 == *p2++) {
				return (char *)p1;
			}
		}
		++p1;
	}
	return NULL;
}
