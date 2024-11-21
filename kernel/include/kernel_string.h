#pragma once
#include "compiler_attributes.h"
#include <stdint.h>
#define NULL ((void *)0)
__nonnull(1, 2) int memcmp(const void *, const void *, uint32_t);
__deprecated("Removed in POSIX.1-2008")
	__nonnull(1, 2) int bcmp(const void *, const void *, uint32_t);
__nonnull(1, 2) void *memcpy(void *, const void *, uint32_t);
void *
memmove(void *, const void *, uint32_t);
__nonnull(1) void *memset(void *, int, uint32_t);
__nonnull(1, 2) char *safestrcpy(char *, const char *, int);
__nonnull(1, 2) char *strlcpy_nostrlen(char *dst, const char *src, int dst_len,
																			 int src_len);
__nonnull(1) uint32_t strlen(const char *);
__nonnull(1, 2) int strncmp(const char *, const char *, uint32_t);
__nonnull(1, 2) char *strncpy(char *, const char *, int);
__nonnull(1, 2) char *strcat(char *dst, const char *src);
