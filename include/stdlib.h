#pragma once
#define EXIT_SUCCESS 0
#define EXIT_FAILURE !EXIT_SUCCESS

#include <bits/__ATEXIT_MAX.h>
#include <bits/__NULL.h>
#include <bits/size_t.h>
#include <bits/wchar_t.h>

typedef __size_t size_t;
typedef __wchar_t wchar_t;

#define NULL __NULL
#define MB_CUR_MAX 1
#define RAND_MAX 32767 // Minimum via POSIX.
#define ATEXIT_MAX __ATEXIT_MAX

// Both qsort and bsearch are implemented as a library
// shared by the kernel and userspace.
void qsort(void *base, size_t nmemb, size_t size,
           int (*)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
              int (*compar)(const void *, const void *));

int rand(void);
void srand(unsigned int seed);

[[noreturn]] void exit(int status);
__attribute__((malloc, alloc_size(1))) void *malloc(size_t);
__attribute__((malloc, alloc_size(1, 2))) void *calloc(size_t nmemb, size_t sz);
__attribute__((malloc, alloc_size(2))) void *realloc(void *ptr, size_t size);
void free(void *);

char *__findenv(const char *name, int len, int *offset);
int putenv(char *str);
char *getenv(const char *name);
int setenv(const char *name, const char *value, int replace);
int unsetenv(const char *name);

int atexit(void (*function)(void));
int atoi(const char *);
long atol(const char *);
long long atoll(const char *);
double atof(const char *nptr);
long strtol(const char *restrict s, char **restrict nptr, int base);
long long strtoll(const char *restrict s, char **restrict nptr, int base);
unsigned long strtoul(const char *s, char **endptr, int base);
unsigned long long strtoull(const char *s, char **endptr, int base);

[[noreturn]] void abort(void);

int system(const char *command);

int mkstemp(char *template);
char *mkdtemp(char *template);

int abs(int i);
long labs(long i);
long long llabs(long long i);
