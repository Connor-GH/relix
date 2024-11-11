#include <stdint.h>
#include "kernel_string.h"
#include "x86.h"
#include "compiler_attributes.h"

__nonnull(1) void *memset(void *dst, int c, uint32_t n)
{
	if ((size_t)dst % 4 == 0 && n % 4 == 0) {
		c &= 0xFF;
		stosl(dst, (c << 24) | (c << 16) | (c << 8) | c, n / 4);
	} else
		stosb(dst, c, n);
	return dst;
}

__nonnull(1, 2) int memcmp(const void *v1, const void *v2, uint32_t n)
{
	const uint8_t *s1, *s2;

	s1 = v1;
	s2 = v2;
	while (n-- > 0) {
		if (*s1 != *s2)
			return *s1 - *s2;
		s1++, s2++;
	}

	return 0;
}
__deprecated("Removed in POSIX.1-2008")
	__nonnull(1, 2) int bcmp(const void *v1, const void *v2, uint32_t n)
{
	return memcmp(v1, v2, n);
}

__nonnull(1, 2) void *memmove(void *dst, const void *src, uint32_t n)
{
	const char *s;
	char *d;

	s = src;
	d = dst;
	if (s < d && s + n > d) {
		s += n;
		d += n;
		while (n-- > 0)
			*--d = *--s;
	} else
		while (n-- > 0)
			*d++ = *s++;

	return dst;
}

// memcpy exists to placate GCC.  Use memmove.
__nonnull(1, 2) void *memcpy(void *dst, const void *src, uint32_t n)
{
	return memmove(dst, src, n);
}

__nonnull(1, 2) char *strcat(char *dst, const char *src)
{
	int start = strlen(dst);
	int j = 0;
	for (int i = start; i < start + strlen(src) + 1; i++, j++) {
		dst[i] = src[j];
	}
	return dst;
}
__nonnull(1, 2) int strncmp(const char *p, const char *q, uint32_t n)
{
	while (n > 0 && *p && *p == *q)
		n--, p++, q++;
	if (n == 0)
		return 0;
	return (uint8_t)*p - (uint8_t)*q;
}

__nonnull(1, 2) char *strncpy(char *s, const char *t, int n)
{
	char *os;

	os = s;
	while (n-- > 0 && (*s++ = *t++) != 0)
		;
	while (n-- > 0)
		*s++ = 0;
	return os;
}

// Like strncpy but guaranteed to NUL-terminate.
__nonnull(1, 2) char *safestrcpy(char *s, const char *t, int n)
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
static inline int
min(int a, int b)
{
	return a > b ? b : a;
}
__nonnull(1, 2) char *strlcpy_nostrlen(char *dst, const char *src, int dst_len,
																			 int src_len)
{
	// this is guaranteed to NUL-terminate even on strings without strlen.
	// only issue is lack of verification of string length.
	return safestrcpy(dst, src, min(dst_len, src_len));
}

__nonnull(1) uint32_t strlen(const char *s)
{
	int n;

	for (n = 0; s[n]; n++)
		;
	return n;
}
