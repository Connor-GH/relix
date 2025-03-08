#pragma once
#include <stddef.h>
char *
strcpy(char *, const char *);
char *
strchr(const char *, char c);
char *
strcat(char *dst, const char *src);
int
strcmp(const char *, const char *);
int
strncmp(const char *p, const char *q, size_t n);
int
strcasecmp(const char *p, const char *q);
int
strncasecmp(const char *p, const char *q, size_t n);
char *
strncpy(char *s, const char *t, size_t n);
char *
stpcpy(char *restrict dst, const char *restrict src);
char *
stpncpy(char *dst, const char *src, size_t n);
char *
__safestrcpy(char *s, const char *t, size_t n);
char *
__strlcpy_nostrlen(char *dst, const char *src, size_t dst_len,
								 size_t src_len);
size_t
strlen(const char *s);
size_t
strnlen(const char *s, size_t size);
char *
strrchr(const char *str, char c);
char *
strstr(const char *s1, const char *s2);
char *
strpbrk(const char *s1, const char *s2);
size_t
strcspn(const char *s1, const char *s2);
size_t
strspn(const char *s1, const char *s2);
char *
strncat(char *dst, const char *src, size_t n);

char *
strsignal(int sig);

void *
memset(void *dst, int c, size_t n);
int
memcmp(const void *v1, const void *v2, size_t n);
void *
memmove(void *dst, const void *src, size_t n);
void *
memcpy(void *dst, const void *src, size_t n);
void *
mempcpy(void *dst, const void *src, size_t n);
char *
strerror(int err_no);
char *
strtok(char *restrict str, const char *restrict delim);
char *
strdup(const char *s);
