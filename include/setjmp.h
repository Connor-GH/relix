#pragma once
#include <signal.h>
#include <stdint.h>

typedef uint64_t __jmp_buf[8];

struct __jmp_buf_tag {
	__jmp_buf __jmpbuf;
	int __mask_was_saved;
	uint64_t __saved_mask[NSIG / (sizeof(uint64_t)) + 1];
};

typedef struct __jmp_buf_tag jmp_buf[1];

typedef jmp_buf sigjmp_buf;

[[gnu::returns_twice]] int setjmp(jmp_buf env);
[[noreturn]] void longjmp(jmp_buf env, int val);

[[gnu::returns_twice]] int sigsetjmp(jmp_buf env, int savemask);
[[noreturn]] void siglongjmp(jmp_buf env, int val);
