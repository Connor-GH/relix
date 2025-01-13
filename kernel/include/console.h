#pragma once
#include "compiler_attributes.h"

extern int echo_out;

void
consoleinit(void);
__attribute__((format(printf, 1, 2))) __nonnull(1) void cprintf(const char *,
																																...);
void
consputc(int);
void
consoleintr(int (*)(void));
__noreturn void
panic(const char *);
