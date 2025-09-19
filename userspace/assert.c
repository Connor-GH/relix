/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Connor-GH. All Rights Reserved.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void
__assert_fail(const char *assertion, const char *file, int lineno,
              const char *func)
{
	fprintf(stderr, "%s:%d: %s: Assertion `%s' failed.\n", file, lineno, func,
	        assertion);
	abort();
}
