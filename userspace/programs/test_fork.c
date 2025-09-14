// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define N 1000

void
forktest(void)
{
	int n;

	fprintf(stdout, "fork test\n");

	for (n = 0; n < N; n++) {
		int pid = fork();
		if (pid < 0) {
			break;
		}
		if (pid == 0) {
			exit(0);
		}
	}

	if (n == N) {
		fprintf(stdout, "fork claimed to work %d times!\n", N);
		exit(0);
	}

	for (; n > 0; n--) {
		if (wait(NULL) < 0) {
			fprintf(stderr, "wait stopped early\n");
			exit(0);
		}
	}

	if (wait(NULL) != -1) {
		fprintf(stderr, "wait got too many\n");
		exit(0);
	}

	fprintf(stdout, "fork test OK\n");
}

int
main(void)
{
	forktest();
	return 0;
}
