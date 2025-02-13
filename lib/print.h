#pragma once
// We just need FILE for its struct impl.
#define __ONLY_SHARE_FILE_IMPL 1
#include <stdio.h>
#undef __ONLY_SHARE_FILE_IMPL
void
sharedlib_vprintf_template(void (*put_function)(FILE *fp, char c, char *buf),
								 size_t (*ansi_func)(const char *), FILE *fp,
								 char *restrict buf, const char *fmt, va_list argp,
								 void (*acq)(void *), void (*rel)(void *), void *lock, bool locking);
