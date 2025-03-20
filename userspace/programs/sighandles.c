#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

static int num = 1;
jmp_buf buf;

// On Ctrl-C, exit with status 32.
void
sigint_handle1(int _)
{
	exit(32);
}
// On Ctrl-C, increment num.
void
sigint_handle2(int _)
{
	num++;
	longjmp(buf, 1);
}


int main(void)
{
	if (signal(SIGINT, sigint_handle2) < 0) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
	while (true) {
		setjmp(buf);
		printf("Got: %d\n", num);
		sleep(100);
	}
	return 0;

}
