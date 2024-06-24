#pragma once

extern int echo_out;

void
consoleinit(void);
__attribute__((format(printf, 1, 2)))
void
cprintf(char *, ...);
void
consoleintr(int (*)(void));
void
panic(char *) __attribute__((noreturn));
