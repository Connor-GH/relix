#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
sigusr(int signum)
{
	_exit(0);
}

int
main(int argc, char **argv)
{
	sighandler_t ret = signal(SIGUSR1, sigusr);
	if (ret == SIG_ERR) {
		fprintf(stderr, "Failed to to signal registration\n");
		exit(EXIT_FAILURE);
	}
	kill(getpid(), SIGUSR1);
	assert(0); // Should NOT get here.
	return 1;
}
