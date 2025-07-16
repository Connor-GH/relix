#include <setjmp.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t num = 1;
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
}

int
main(void)
{
	if (signal(SIGINT, sigint_handle2) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
	while (true) {
		printf("Got: %d\n", num);
		sleep(100);
	}
	return 0;
}
