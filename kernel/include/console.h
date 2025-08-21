#pragma once
#if __KERNEL__
#include "lib/compiler_attributes.h"
#include "spinlock.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

extern int echo_out;

void consoleinit(void);
__attribute__((format(printf, 1, 2))) __nonnull(1) void cprintf(const char *,
                                                                ...);
__attribute__((format(printf, 1, 2)))
__nonnull(1) void vga_cprintf(const char *fmt, ...);
__attribute__((format(printf, 1, 2)))
__nonnull(1) void uart_printf(const char *fmt, ...);
__attribute__((format(printf, 1, 2)))
__nonnull(1) void early_uart_printf(const char *fmt, ...);
__attribute__((format(printf, 2, 3)))
__nonnull(1) void ksprintf(char *restrict str, const char *fmt, ...);

#if __KERNEL_DEBUG__ && defined(__FILE_NAME__)
#define pr_debug_file(...) uart_printf(__FILE_NAME__ ": " __VA_ARGS__)
#define pr_debug(...) uart_printf(__VA_ARGS__)
#else
#define pr_debug(...)
#define pr_debug_file(...)
#endif

void consputc(int);
void consoleintr(int (*)(void));
__cold void panic_print_before(void);
__noreturn __cold void panic_print_after(void);
#define panic(...)          \
	panic_print_before();     \
	vga_cprintf(__VA_ARGS__); \
	panic_print_after()

int kernel_vprintf_template(void (*put_function)(char c, char *buf),
                            size_t (*ansi_func)(const char *),
                            char *restrict buf, const char *fmt, va_list argp,
                            struct spinlock *lock, bool locking,
                            size_t print_n_chars);
struct termios *get_term_settings(int minor);
void set_term_settings(int minor, struct termios *termios);
#endif
