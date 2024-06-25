#pragma once
#include "compiler_attributes.h"
#include <types.h>
__nonnull(1, 2)
int
memcmp(const void *, const void *, uint);
__nonnull(1, 2)
void *
memmove(void *, const void *, uint);
__nonnull(1)
void *
memset(void *, int, uint);
__nonnull(1, 2)
char *
safestrcpy(char *, const char *, int);
__nonnull(1)
uint
strlen(const char *);
__nonnull(1, 2)
int
strncmp(const char *, const char *, uint);
__nonnull(1, 2)
char *
strncpy(char *, const char *, int);

