#pragma once
#include "compiler_attributes.h"

extern int echo_out;

void
consoleinit(void);
__attribute__((format(printf, 1, 2))) __nonnull(1) void cprintf(const char *,
																																...);
__attribute__((format(printf, 1, 2)))
__nonnull(1) void vga_cprintf(const char *fmt, ...);
__attribute__((format(printf, 1, 2)))
__nonnull(1) void uart_cprintf(const char *fmt, ...);
__attribute__((format(printf, 2, 3)))
__nonnull(1) void ksprintf(char *restrict str, const char *fmt, ...);
void
consputc(int);
void
consoleintr(int (*)(void));
__noreturn void
panic(const char *);
