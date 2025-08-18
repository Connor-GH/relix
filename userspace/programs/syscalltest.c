#include "__intrinsics.h"
#include "libc_syscalls.h"
#include <stdint.h>
#include <stdio.h>

#define NUM_ITERS 10000
int
main(void)
{
	for (size_t i = 0; i < NUM_ITERS; i++) {
		uint64_t before_ticks = rdtsc();
		int ret = (int)__syscall0(255);
		uint64_t after_ticks = rdtsc();
		printf("%ld\n", after_ticks - before_ticks);
	}
	return 0;
}
