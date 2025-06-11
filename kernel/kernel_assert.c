#include "console.h"
#include <kernel_assert.h>

__attribute__((noreturn)) void
kernel_assert_fail(const char *assertion, const char *file, int lineno,
                   const char *func)
{
	uart_printf("%s:%d: %s: Assertion `%s' failed.\n", file, lineno, func,
	            assertion);
	uart_printf("Aborting.\n");
	panic("Assertion failed.");
}
