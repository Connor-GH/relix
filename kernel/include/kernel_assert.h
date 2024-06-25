#pragma once
#include "compiler_attributes.h"
#include <types.h>
#include <console.h>

static __always_inline void
kernel_assert_fail(const char *assertion, const char *file,
		int lineno, const char *func)
{
	cprintf("%s:%d: %s: Assertion `%s' failed.\n", file, lineno, func,
			assertion);
	cprintf("Aborting.\n");
	panic("Assertion failed.");
}
// the no-op here makes clang happy.
static inline void
no_op(void)
{
}
#define kernel_assert(expr) (expr ? no_op() : kernel_assert_fail(#expr, __FILE__, __LINE__, __func__))
