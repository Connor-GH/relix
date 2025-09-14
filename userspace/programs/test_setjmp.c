#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>

jmp_buf buf;
volatile int var = 0;

void
func(void)
{
	var = 2;

	longjmp(buf, 1);
	printf("Should NOT print\n");
}
int
main(void)
{
	if (setjmp(buf)) {
		var = 3;
	} else {
		var = 1;
		func();
	}
	assert(var == 3);
	printf("TEST PASSED\n");
	return 0;
}
