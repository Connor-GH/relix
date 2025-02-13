#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "kernel/include/x86.h"
#include <stddef.h>

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
	while (*p && *p == *q)
		p++, q++;
	return (uint8_t)*p - (uint8_t)*q;
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
size_t
strlen(const char *s)
{
	int n;

	for (n = 0; s[n]; n++)
		;
	return n;
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

char *
strncpy(char *dst, const char *src, size_t n)
{
	char *my_dst;

	my_dst = dst;
	while (n-- > 0 && (*dst++ = *src++) != 0)
		; //*my_dst++ = *src++;
	while (n-- > 1)
		*dst++ = '\0';
	return my_dst;
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
	if ((size_t)dst % 4 == 0 && n % 4 == 0) {
		c &= 0xFF;
		stosl(dst, (c << 24) | (c << 16) | (c << 8) | c, n / 4);
	} else
		stosb(dst, c, n);
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
	// If source is lower than dest in memory,
	// we don't have to worry about clobbering it going forwards.
	if (dest <= source) {
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
memcpy(void *dst, const void *src, size_t n)
{
	if (n % 8 == 0)
		return movsq((uint64_t *)dst, (uint64_t *)src, n / sizeof(uint64_t));
	else
		return movsb((uint8_t *)dst, (uint8_t *)src, n / sizeof(uint8_t));
}
char *
strcat(char *dst, const char *src)
{
	return strncat(dst, src, strlen(src) + 1);
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
