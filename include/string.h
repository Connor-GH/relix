#pragma once
#include <stdint.h>
char *
strcpy(char *, const char *);
char *
strchr(const char *, char c);
char *
strcat(char *dst, const char *src);
int
strcmp(const char *, const char *);
int
strncmp(const char *p, const char *q, uint32_t n);
char *
strncpy(char *s, const char *t, int n);
#ifdef __KERNEL__
char *
safestrcpy(char *s, const char *t, int n);
#endif
uint32_t
strlen(const char *s);
char *
strchr(const char *str, char c);
char *
strrchr(const char *str, char c);

void *
memset(void *dst, int c, uint32_t n);
int
memcmp(const void *v1, const void *v2, uint32_t n);
void *
memmove(void *dst, const void *src, uint32_t n);
void *
memcpy(void *dst, const void *src, uint32_t n);
const char *const
strerror(int err_no);
