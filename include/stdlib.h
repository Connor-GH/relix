#pragma once
#define EXIT_SUCCESS 0
#define EXIT_FAILURE !EXIT_SUCCESS

#include <bits/NULL.h>
#include <bits/size_t.h>
#include <bits/wchar_t.h>
typedef __size_t size_t;
typedef __wchar_t wchar_t;
// qsort is implemented as a library shared
// by the kernel and userspace.
void qsort(void *base, size_t nmemb, size_t size,
           int (*)(const void *, const void *));

#ifndef __KERNEL__

#define RAND_MAX 32767 // Minimum via POSIX.
int rand(void);
void srand(unsigned int seed);
__attribute__((noreturn)) void exit(int status);
__attribute__((malloc)) void *malloc(size_t);
__attribute__((malloc)) void *calloc(size_t nmemb, size_t sz);
void free(void *);
int atoi_base(const char *, int base);
__attribute__((malloc)) void *realloc(void *ptr, size_t size);

char *__findenv(const char *name, int len, int *offset);
int putenv(char *str);
char *getenv(const char *name);
int setenv(const char *name, const char *value, int replace);
int unsetenv(const char *name);

// POSIX.1 requires at least 32 atexit handlers.
#define ATEXIT_MAX 32
int atexit(void (*function)(void));
int atoi(const char *);
long atol(const char *);
long long atoll(const char *);
double atof(const char *nptr);
long strtol(const char *restrict s, char **restrict nptr, int base);
long long strtoll(const char *restrict s, char **restrict nptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
unsigned long long strtoull(const char *s, char **endptr, int base);
__attribute__((noreturn)) void abort(void);
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));
int system(const char *command);
#endif
