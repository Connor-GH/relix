#include "kernel/include/kernel_signal.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
sigalrm_handler(int sig)
{
}

int
main(int argc, char **argv)
{
	if (argc == 1) {
		fprintf(stderr, "usage: %s [seconds]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (signal(SIGALRM, sigalrm_handler) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	int ret = sleep(atoi(argv[1]));
	while (ret != 0) {
		ret = sleep(ret);
	}
	return 0;
}
