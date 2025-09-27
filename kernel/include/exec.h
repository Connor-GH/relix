#pragma once
#if __RELIX_KERNEL__
#include "lib/compiler_attributes.h"

__nonnull(1, 2) int execve(char *, char *const[], char *const[]);
#endif
