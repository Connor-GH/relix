#pragma once
#define EXIT_SUCCESS 0
#define EXIT_FAILURE !EXIT_SUCCESS
#ifndef __KERNEL__
#include <stddef.h>
#include <stdint.h>
__attribute__((noreturn)) void
exit(int status);
__attribute__((malloc)) void *
malloc(size_t);
__attribute__((malloc)) void *
calloc(size_t nmemb, size_t sz);
void
free(void *);
int
atoi_base(const char *, uint32_t base);
__attribute__((malloc)) void *
realloc(void *ptr, size_t size);
void
qsort(void *base, size_t nmemb, size_t size,
			int (*)(const void *, const void *));
char *
getenv(const char *name);
int
setenv(const char *name, const char *value, int replace);
// POSIX.1 requires at least 32 atexit handlers.
#define ATEXIT_MAX 32
int
atexit(void (*function)(void));
int
atoi(const char *);
long
atol(const char *);
long long
atoll(const char *);
double
atof(const char *nptr);
long
strtol(const char *restrict s, char **restrict nptr, int base);
long long
strtoll(const char *restrict s, char **restrict nptr, int base);
__attribute__((noreturn)) void
abort(void);
void *
bsearch(const void *key, const void *base, size_t nmemb, size_t size,
				int (*compar)(const void *, const void *));
int
system(const char *command);
#endif
