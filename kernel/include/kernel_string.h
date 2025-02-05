#pragma once
#include "compiler_attributes.h"
#include <stddef.h>
#define NULL ((void *)0)
__nonnull(1, 2) int memcmp(const void *, const void *, size_t);
__deprecated("Removed in POSIX.1-2008")
	__nonnull(1, 2) int bcmp(const void *, const void *, size_t);
__nonnull(1, 2) void *memcpy(void *, const void *, size_t);
void *
memmove(void *, const void *, size_t);
__nonnull(1) void *memset(void *, int, size_t);
__nonnull(1, 2) char *safestrcpy(char *, const char *, int);
__nonnull(1, 2) char *strlcpy_nostrlen(char *dst, const char *src, size_t dst_len,
																			 size_t src_len);
__nonnull(1) size_t strlen(const char *);
__nonnull(1, 2) int strncmp(const char *, const char *, size_t);
__nonnull(1, 2) char *strncpy(char *, const char *, size_t);
__nonnull(1, 2) char *strcat(char *dst, const char *src);
