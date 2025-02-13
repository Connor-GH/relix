#pragma once

typedef struct {
	unsigned long __val[1024 / (8 * sizeof(unsigned long))];
} __sigset_t;
typedef long __jmp_buf[8];

struct __jmp_buf_tag {
	__jmp_buf __jmpbuf;
	int __mask_was_saved;
	__sigset_t __saved_mask;
};

typedef struct __jmp_buf_tag jmp_buf[1];

int
setjmp(jmp_buf env);
void
longjmp(struct __jmp_buf_tag env[1], int val) __attribute__((noreturn));
