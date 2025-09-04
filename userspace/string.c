#include <__intrinsics.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	return strncmp(p, q, min(np, nq) + 1);
}

// We do not currently support locales outside of C.
int
strcoll(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
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
	if (__unlikely(n == 0)) {
		return 0;
	}
	while (n > 0 && *p && *p == *q) {
		n--, p++, q++;
	}
	if (n == 0) {
		return 0;
	}
	return (uint8_t)*p - (uint8_t)*q;
}

int
strncasecmp(const char *p, const char *q, size_t n)
{
	if (__unlikely(n == 0)) {
		return 0;
	}
	while (n > 0 && *p && tolower(*p) == tolower(*q)) {
		n--, p++, q++;
	}
	if (n == 0) {
		return 0;
	}
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
	size_t n;

	for (n = 0; s[n]; n++)
		;
	return n;
}
size_t
strnlen(const char *s, size_t size)
{
	size_t n;
	if (__unlikely(size == 0)) {
		return 0;
	}

	for (n = 0; n < size && s[n]; n++)
		;
	return n;
}

char *
strchr(const char *s, int c)
{
	for (; *s; s++) {
		if (*s == c) {
			return (char *)s;
		}
	}
	return NULL;
}

void *
memchr(const void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (((const char *)s)[i] == c) {
			return (void *)s + i;
		}
	}
	return NULL;
}

void *
memrchr(const void *s, int c, size_t n)
{
	for (size_t i = n; i > 0; i--) {
		if (((const char *)s)[i - 1] == c) {
			return (char *)s + i - 1;
		}
	}
	return NULL;
}

char *
strrchr(const char *s, int c)
{
	return memchr(s, c, strlen(s) + 1);
}

// This sufficiently handles the case where
// strncpy(NULL, NULL, 0) is called, as it
// returns dst. This is needed due to C2y.
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
	if (__unlikely(n == 0)) {
		return dst;
	}
	while (n-- > 0 && (*dst++ = *src++) != 0)
		;
	while (n-- > 1) {
		*dst++ = '\0';
	}
	return dst;
}

static char *__strtok_token = NULL;
/*
 * Break a string into a sequence of zero or more nonempty tokens.
 * If the same string is being parsed as the previous invocation, str must be
 * NULL.
 */
char *
strtok_r(char *restrict str, const char *restrict delim,
         char **restrict saveptr)
{
	if (str != NULL) {
		*saveptr = str;
	}

	if (*saveptr == NULL) {
		return NULL;
	}

	while (**saveptr && strchr(delim, **saveptr) != NULL) {
		(*saveptr)++;
	}

	if (**saveptr == '\0') {
		return NULL;
	}

	char *start = *saveptr;
	while (**saveptr && strchr(delim, **saveptr) == NULL) {
		(*saveptr)++;
	}

	// The input string gets consumed from the input, so we modify it here.
	if (**saveptr) {
		// C does not recognize "(**saveptr)++" as an lvalue.
		*(*saveptr)++ = '\0';
	}

	return start;
}

char *
strtok(char *restrict str, const char *restrict delim)
{
	return strtok_r(str, delim, &__strtok_token);
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
	if (__unlikely(n == 0)) {
		return 0;
	}

	dst = v1;
	s2 = v2;
	while (n-- > 0) {
		if (*dst != *s2) {
			return *dst - *s2;
		}
		dst++, s2++;
	}

	return 0;
}

// Based on code from https://libc11.org/string/memmove.html (public domain)
void *
memmove(void *dst, const void *src, size_t n)
{
	char *dest = (char *)dst;
	const char *source = (const char *)src;
	if (__unlikely(dest == source || n == 0)) {
		return dst;
	}

	if (__unlikely(dest == NULL)) {
		return NULL;
	}
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

// Since C2y defines NULL+0 == NULL and our memcpy follows
// the C2y 0-length no-op, this is well-defined.
void *
mempcpy(void *dst, const void *src, size_t n)
{
	return memcpy(dst, src, n) + n;
}

void *
memcpy(void *dst, const void *src, size_t n)
{
	return movsb(dst, src, n);
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
	if (__unlikely(n == 0)) {
		return dst;
	}
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
	if (n <= 0) {
		return os;
	}
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

// Duplicate a string
// Caller frees the string.
char *
strndup(const char *s, size_t n)
{
	if (__unlikely(s == NULL || n == 0)) {
		return (char *)s;
	}
	char *new_s = malloc(n);
	if (new_s == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	strncpy(new_s, s, n);
	return new_s;
}

char *
strdup(const char *s)
{
	return strndup(s, strlen(s) + 1);
}
