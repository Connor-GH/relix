#pragma once
#include <bits/__NULL.h>
#include <bits/size_t.h>

typedef __size_t size_t;
#define NULL __NULL

#ifndef __NONNULL
#define __NONNULL(...) __attribute__((__nonnull__(__VA_ARGS__)))
#endif

// As of wg14 draft 3322, zero-length operations on
// NULL pointers are defined as no-ops. GNU glibc
// has those as undefined, which is incorrect
// behavior as of C2y.

char *strcpy(char *dst, const char *src) __NONNULL(1, 2);
char *strchr(const char *str, int c) __NONNULL(1);
char *strrchr(const char *str, int c) __NONNULL(1);
char *strcat(char *dst, const char *src) __NONNULL(1, 2);
int strcmp(const char *s1, const char *s2) __NONNULL(1, 2);
int strncmp(const char *p, const char *q, size_t n);
int strcasecmp(const char *p, const char *q) __NONNULL(1, 2);
int strncasecmp(const char *p, const char *q, size_t n);
char *strncpy(char *s, const char *t, size_t n);
char *stpcpy(char *restrict dst, const char *restrict src) __NONNULL(1, 2);
char *stpncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *s) __NONNULL(1);
size_t strnlen(const char *s, size_t size);
char *strstr(const char *s1, const char *s2) __NONNULL(1, 2);
char *strpbrk(const char *s1, const char *s2) __NONNULL(1, 2);
size_t strcspn(const char *s1, const char *s2) __NONNULL(1, 2);
size_t strspn(const char *s1, const char *s2) __NONNULL(1, 2);
char *strncat(char *dst, const char *src, size_t n);

char *strsignal(int sig);

void *memset(void *dst, int c, size_t n);
int memcmp(const void *v1, const void *v2, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *mempcpy(void *dst, const void *src, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);
char *strerror(int err_no);
char *strtok(char *restrict str, const char *restrict delim) __NONNULL(2);
char *strdup(const char *s) __NONNULL(1);
char *strndup(const char *s, size_t n);

int strcoll(const char *s1, const char *s2) __NONNULL(1, 2);

// Nonstandard
char *__safestrcpy(char *s, const char *t, size_t n);
char *__strlcpy_nostrlen(char *dst, const char *src, size_t dst_len,
                         size_t src_len);
