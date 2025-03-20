#include <unistd.h>
#include <stdio.h>
#include <setjmp.h>

jmp_buf buf;

void
func(void)
{
	printf("2\n");

	longjmp(buf, 1);
	printf("Should NOT print\n");
}
int
main(void)
{
	if (setjmp(buf)) {
		printf("3\n");
	} else {
		printf("1\n");
		func();
	}
	return 0;

}
