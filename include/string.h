#pragma once
#include <types.h>

char *
strcpy(char *, const char *);
char *
strchr(const char *, char c);
char *
strcat(char *dst, const char *src);
int
strcmp(const char *, const char *);
int
strncmp(const char *p, const char *q, uint n);
char *
strncpy(char *s, const char *t, int n);
#ifdef __KERNEL__
char *
safestrcpy(char *s, const char *t, int n);
#endif
uint
strlen(const char *s);
char *
strchr(const char *str, char c);
char *
strrchr(const char *str, char c);

void *
memset(void *dst, int c, uint n);
int
memcmp(const void *v1, const void *v2, uint n);
void *
memmove(void *dst, const void *src, uint n);
void *
memcpy(void *dst, const void *src, uint n);
