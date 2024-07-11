#include <stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
int
main(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		fprintf(stderr, "usage: kill pid...\n");
		return 1;
	}
	for (i = 1; i < argc; i++)
		kill(atoi(argv[i]));
	return 0;
}
