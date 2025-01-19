#include "console.h"
#include <kernel_assert.h>

__attribute__((noreturn)) void
kernel_assert_fail(const char *assertion, const char *file, int lineno,
									 const char *func)
{
	cprintf("%s:%d: %s: Assertion `%s' failed.\n", file, lineno, func, assertion);
	cprintf("Aborting.\n");
	panic("Assertion failed.");
}
