#pragma once
// We just need FILE for its struct impl.
#include <stdio.h>
int
__libc_vprintf_template(void (*put_function)(FILE *fp, char c, char *buf),
								 size_t (*ansi_func)(const char *), FILE *fp,
								 char *restrict buf, const char *fmt, va_list argp,
								 size_t print_n_chars);
